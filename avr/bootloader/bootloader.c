/*
 * =====================================================================================
 *
 * ________        .__        __    ________               ____  ________
 * \_____  \  __ __|__| ____ |  | __\______ \   _______  _/_   |/  _____/
 *  /  / \  \|  |  \  |/ ___\|  |/ / |    |  \_/ __ \  \/ /|   /   __  \
 * /   \_/.  \  |  /  \  \___|    <  |    `   \  ___/\   / |   \  |__\  \
 * \_____\ \_/____/|__|\___  >__|_ \/_______  /\___  >\_/  |___|\_____  /
 *        \__>             \/     \/        \/     \/                 \/
 *
 *                                  www.optixx.org
 *
 *
 *        Version:  1.0
 *        Created:  07/21/2009 03:32:16 PM
 *         Author:  david@optixx.org
 *
 *        simple USBasp compatible bootloader by 
 *        Alexander Neumann <alexander@lochraster.org> 
 *        inspired by USBasploader by Christian Starkjohann,
 * =====================================================================================
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>
#include <avr/wdt.h>

#include "config.h"
#include "usbdrv/usbdrv.c"





/*
 * USBasp requests, taken from the original USBasp sourcecode 
 */
#define USBASP_FUNC_CONNECT     1
#define USBASP_FUNC_DISCONNECT  2
#define USBASP_FUNC_TRANSMIT    3
#define USBASP_FUNC_READFLASH   4
#define USBASP_FUNC_ENABLEPROG  5
#define USBASP_FUNC_WRITEFLASH  6
#define USBASP_FUNC_READEEPROM  7
#define USBASP_FUNC_WRITEEEPROM 8
#define USBASP_FUNC_SETLONGADDRESS 9

/*
 * additional functions 
 */
#define FUNC_ECHO               0x17

/*
 * atmel isp commands 
 */
#define ISP_CHIP_ERASE1         0xAC
#define ISP_CHIP_ERASE2         0x80
#define ISP_READ_SIGNATURE      0x30
#define ISP_READ_EEPROM         0xa0
#define ISP_WRITE_EEPROM        0xc0



#define LED_PORT	            PORTC
#define LED_DIR		            DDRC
#define LED_PIN		            PC7

#define DLED_ON  {((LED_PORT &=~ (1 << LED_PIN)),\
                  (LED_DIR &=~ (1 << LED_PIN))); }
#define DLED_OFF {((LED_PORT &=~ (1 << LED_PIN)),\
                   (LED_DIR |= (1 << LED_PIN))); }
#define DLED_TGL {((LED_PORT &=~ (1 << LED_PIN)),\
                  (LED_DIR ^= (1 << LED_PIN)));}


#define AVR_BTLDR_EN_PORT	    PORTC
#define AVR_BTLDR_EN_DIR	    DDRC
#define AVR_BTLDR_EN_PIN	    PC1
#define AVR_BTLDR_EN_IN	        PINC

#undef AVR_BTLDR_SWITCH
/*
 * some predefined signatures, taken from the original USBasp sourcecode 
 */
static const uint8_t signature[4] = {
#ifdef SIGNATURE_BYTES
    SIGNATURE_BYTES
#elif defined (__AVR_ATmega8__) || defined (__AVR_ATmega8HVA__)
    0x1e, 0x93, 0x07, 0
#elif defined (__AVR_ATmega48__) || defined (__AVR_ATmega48P__)
    0x1e, 0x92, 0x05, 0
#elif defined (__AVR_ATmega88__) || defined (__AVR_ATmega88P__)
    0x1e, 0x93, 0x0a, 0
#elif defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__)
    0x1e, 0x94, 0x06, 0
#elif defined (__AVR_ATmega328P__)
    0x1e, 0x95, 0x0f, 0
#elif defined (__AVR_ATmega644__)
    0x1e, 0x96, 0x09, 0
#else
#   error "Device signature is not known, please edit config.h!"
#endif
};

#ifndef BOOT_SECTION_START
#   error "BOOT_SECTION_START undefined!"
#endif

#if defined (__AVR_ATmega644__)
    /*
     * Due arvdude limitations we can't erase the whole progmem without running into an usb timeount on cleint side. So we we limit the
     * erase section by 0x1000 
     */
#define ERASE_SECTION 0xe000
#else
#define ERASE_SECTION BOOT_SECTION_START
#endif


#ifdef DEBUG_UART
static __attribute__ ((__noinline__))
     void uart_putc(uint8_t data)
{
    while (!(UCSR0A & _BV(UDRE0)));
    UDR0 = data;
}
#else
#define uart_putc(x)
#endif

#ifdef DEBUG_UART
static __attribute__ ((__noinline__))
     void uart_puts(uint8_t * data)
{
    while (*data) {
        uart_putc(*data);
        data++;
    }
}
#else
#define uart_puts(x)
#endif


/*
 * supply custom usbDeviceConnect() and usbDeviceDisconnect() macros which turn the interrupt on and off at the right times, and prevent
 * the execution of an interrupt while the pullup resistor is switched off 
 */

#ifdef USB_CFG_PULLUP_IOPORTNAME
#undef usbDeviceConnect
#define usbDeviceConnect()      do { \
                                    USB_PULLUP_DDR |= (1<<USB_CFG_PULLUP_BIT); \
                                    USB_PULLUP_OUT |= (1<<USB_CFG_PULLUP_BIT); \
                                    USB_INTR_ENABLE |= (1 << USB_INTR_ENABLE_BIT); \
                                   } while(0);
#undef usbDeviceDisconnect
#define usbDeviceDisconnect()   do { \
                                    USB_INTR_ENABLE &= ~(1 << USB_INTR_ENABLE_BIT); \
                                    USB_PULLUP_DDR &= ~(1<<USB_CFG_PULLUP_BIT); \
                                    USB_PULLUP_OUT &= ~(1<<USB_CFG_PULLUP_BIT); \
                                   } while(0);
#endif

/*
 * prototypes 
 */
void __attribute__ ((__noreturn__, __noinline__,
                     __naked__)) leave_bootloader(void);

/*
 * we just support flash sizes <= 64kb, for code size reasons if you need to program bigger devices, have a look at USBasploader:
 * http://www.obdev.at/products/avrusb/usbasploader.html 
 */
#if FLASHEND > 0xffff
#   error "usbload only supports up to 64kb of flash!"
#endif

/*
 * we are just checking the lower byte of flash_address, so make sure SPM_PAGESIZE is <= 256 
 */
#if SPM_PAGESIZE > 256
#   error "SPM_PAGESIZE is too big (just checking lower byte)"
#endif

/*
 * start flash (byte address) read/write at this address 
 */
usbWord_t flash_address;
uint8_t bytes_remaining;
uint8_t request;
uint8_t request_exit;

uint8_t timeout;



usbMsgLen_t usbFunctionSetup(uchar data[8])
{
    usbRequest_t *req = (void *) data;
    uint8_t len = 0;
    static uint8_t buf[4];

    /*
     * set global data pointer to local buffer 
     */
    usbMsgPtr = buf;

    /*
     * on enableprog just return one zero, which means success 
     */
    if (req->bRequest == USBASP_FUNC_ENABLEPROG) {
        buf[0] = 0;
        len = 1;
        timeout = 255;
    } else if (req->bRequest == USBASP_FUNC_CONNECT) {
        /*
         * turn on led 
         */
        DLED_ON;
    } else if (req->bRequest == USBASP_FUNC_DISCONNECT) {
        /*
         * turn off led 
         */
        DLED_OFF;
        request_exit = 1;
        /*
         * catch query for the devicecode, chip erase and eeprom byte requests 
         */
    } else if (req->bRequest == USBASP_FUNC_TRANSMIT) {

        /*
         * reset buffer with zeroes 
         */
        memset(buf, '\0', sizeof(buf));

        /*
         * read the address for eeprom operations 
         */
        usbWord_t address;
        address.bytes[0] = data[4];     /* low byte is data[4] */
        address.bytes[1] = data[3];     /* high byte is data[3] */

        /*
         * if this is a request to read the device signature, answer with the appropiate signature byte 
         */
        if (data[2] == ISP_READ_SIGNATURE) {
            /*
             * the complete isp data is reported back to avrdude, but we just need byte 4 bits 0 and 1 of byte 3 determine the signature
             * byte address 
             */
            buf[3] = signature[data[4] & 0x03];

#ifdef ENABLE_CATCH_EEPROM_ISP
            /*
             * catch eeprom read 
             */
        } else if (data[2] == ISP_READ_EEPROM) {

            buf[3] = eeprom_read_byte((uint8_t *) address.word);

            /*
             * catch eeprom write 
             */
        } else if (data[2] == ISP_WRITE_EEPROM) {

            /*
             * address is in data[4], data[3], and databyte is in data[5] 
             */
            eeprom_write_byte((uint8_t *) address.word, data[5]);
#endif

            /*
             * catch a chip erase 
             */
        } else if (data[2] == ISP_CHIP_ERASE1 && data[3] == ISP_CHIP_ERASE2) {
            uart_puts("\n\rErase Flash");
            for (flash_address.word = 0;
                 flash_address.word < ERASE_SECTION;
                 flash_address.word += SPM_PAGESIZE) {

                /*
                 * wait and erase page 
                 */
                boot_spm_busy_wait();
                if (flash_address.word && flash_address.word % 1024 == 0)
                    uart_putc('.');
                cli();
                boot_page_erase(flash_address.word);
                sei();
            }
            uart_puts("\n\rWrite Flash");
        }

        /*
         * in case no data has been filled in by the if's above, just return zeroes 
         */
        len = 4;

#ifdef ENABLE_ECHO_FUNC
        /*
         * implement a simple echo function, for testing the usb connectivity 
         */
    } else if (req->bRequest == FUNC_ECHO) {
        buf[0] = req->wValue.bytes[0];
        buf[1] = req->wValue.bytes[1];
        len = 2;
#endif
    } else if (req->bRequest >= USBASP_FUNC_READFLASH) {
        /*
         * && req->bRequest <= USBASP_FUNC_SETLONGADDRESS 
         */
        /*
         * extract address and length 
         */
        flash_address.word = req->wValue.word;
        bytes_remaining = req->wLength.bytes[0];
        request = req->bRequest;
        /*
         * hand control over to usbFunctionRead()/usbFunctionWrite() 
         */
        len = 0xff;
    }

    return len;
}

uchar usbFunctionWrite(uchar * data, uchar len)
{
    if (len > bytes_remaining)
        len = bytes_remaining;
    bytes_remaining -= len;
    if (request == USBASP_FUNC_WRITEEEPROM) {
        for (uint8_t i = 0; i < len; i++)
            eeprom_write_byte((uint8_t *) flash_address.word++, *data++);
    } else {
        /*
         * data is handled wordwise, adjust len 
         */
        len /= 2;
        len -= 1;
        for (uint8_t i = 0; i <= len; i++) {
            uint16_t *w = (uint16_t *) data;
            cli();
            boot_page_fill(flash_address.word, *w);
            sei();

            usbWord_t next_address;
            next_address.word = flash_address.word;
            next_address.word += 2;
            data += 2;

            /*
             * write page if page boundary is crossed or this is the last page 
             */
            if (next_address.bytes[0] % SPM_PAGESIZE == 0 ||
                (bytes_remaining == 0 && i == len)) {
                cli();
                boot_page_write(flash_address.word);
                sei();
                boot_spm_busy_wait();
                cli();
                boot_rww_enable();
                sei();
            }

            flash_address.word = next_address.word;
        }
    }

    /*
     * flash led on activity 
     */
    DLED_TGL;

    return (bytes_remaining == 0);
}

uchar usbFunctionRead(uchar * data, uchar len)
{
    if (len > bytes_remaining)
        len = bytes_remaining;
    bytes_remaining -= len;

    for (uint8_t i = 0; i < len; i++) {
        if (request == USBASP_FUNC_READEEPROM)
            *data = eeprom_read_byte((void *) flash_address.word);
        else
            *data = pgm_read_byte_near((void *) flash_address.word);
        data++;
        flash_address.word++;
    }

    /*
     * flash led on activity 
     */
    DLED_TGL;

    return len;
}

void (*jump_to_app) (void) = 0x0000;
void leave_bootloader(void)
{

    cli();

    /*
     * disconnect usb 
     */
    usbDeviceDisconnect();
#if 0    
    for (uint8_t i = 0; i < 50; i++)
        _delay_ms(10);          /* 0 means 0x10000, 38*1/f*0x10000 =~ 498ms */
#endif
    /*
     * enable watchdog to soft-reset the uC for clean startup of new application 
     */
    wdt_enable(WDTO_15MS);

    /*
     * let watchdog kick in and reset uC 
     */
    while (1);

}

void banner(){
    uart_puts("\n\r");
    uart_puts("\n\r");
    uart_puts("\n\r");
    uart_puts("Quickdev16 Bootloader v0.2\n\r"); 
    uart_puts("www.optixx.org\n\r");
}

int __attribute__ ((noreturn, OS_main)) main(void)
{
    /*
     * start bootloader 
     */

#ifdef DEBUG_UART
    /*
     * init uart (115200 baud, at 20mhz) 
     */
    UBRR0L = 10;
    UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
    UCSR0B = _BV(TXEN0);
#endif

    uint8_t reset = MCUSR;
    uint16_t delay = 0;
    timeout = TIMEOUT;

    DDRC &= ~(1 << AVR_BTLDR_EN_PIN);	    
    PORTC &= ~(1 << AVR_BTLDR_EN_PIN);	    

    /*
     * if watchdog reset, disable watchdog and jump to app 
     */
    if (reset & _BV(WDRF)) {
        uart_puts("Found watchdog reset\n\r");
        MCUSR = 0;
        wdt_disable();
        uart_puts("Jump to 0x0000\n\r");
        jump_to_app();
    }

#if AVR_BTLDR_SWITCH_ENABLE
    if ((AVR_BTLDR_EN_IN & ( 1 << AVR_BTLDR_EN_PIN)) == 0){
        banner();
        uart_puts("Bootloader flashing is disabled\n\r");
        MCUSR = 0;
        leave_bootloader();
    }
#endif

    /*
     * if power-on reset, quit bootloader via watchdog reset 
     */
    if (reset & _BV(PORF)) {
        banner();
        uart_puts("Found power on reset\n\r");
        MCUSR = 0;
        leave_bootloader();
    }
    banner();
    uart_puts("Enter programming mode\n\r");
    /*
     * else: enter programming mode 
     */

    /*
     * clear external reset flags 
     */
    MCUSR = 0;

    /*
     * init exit request state 
     */
    request_exit = 0;

    /*
     * move interrupts to boot section 
     */

    MCUCR = (1 << IVCE);
    MCUCR = (1 << IVSEL);

    /*
     * enable interrupts 
     */
    sei();

    /*
     * initialize usb pins 
     */
    usbInit();


    /*
     * disconnect for ~500ms, so that the host re-enumerates this device 
     */
    usbDeviceDisconnect();
    for (uint8_t i = 0; i < 50; i++)
        _delay_ms(10);          /* 0 means 0x10000, 38*1/f*0x10000 =~ 498ms */
    usbDeviceConnect();
    uart_puts("Wait for firmware");
    while (1) {
        usbPoll();

        delay++;

        /*
         * do some led blinking, so that it is visible that the bootloader is still running 
         */
        if (delay == 0) {
            uart_putc('.');
            DLED_TGL;
            if (timeout < 255)
                timeout--;
        }

        if (request_exit || timeout == 0) {
            uart_puts("\n\rExit\n\r");
            _delay_ms(10);
            leave_bootloader();
        }

    }

}
