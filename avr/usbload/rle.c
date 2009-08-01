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
 * =====================================================================================
 */

 
#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/pgmspace.h>       /* required by usbdrv.h */
#include <util/delay.h>         /* for _delay_ms() */
#include <avr/interrupt.h>      /* for sei() */

#include "sram.h"
#include "debug.h"

#define RUNCHAR 0x90

uint8_t rle_decode(PGM_VOID_P in_addr, int32_t in_len, uint32_t out_addr)
{
	uint8_t in_byte, in_repeat, last_byte;
	uint32_t out_len, out_len_left;
    printf("RLE decode len=%li addr=0x%08lx\n",in_len,out_addr);
    
	out_len_left = out_len;
    sram_bulk_write_start(out_addr);
#define INBYTE(b) \
	do { \
	         if ( --in_len < 0 ) { \
			   return 1; \
		 } \
         cli();\
         b = pgm_read_byte((PGM_VOID_P)in_addr++);  \
         sei();\
	} while(0)

#define OUTBYTE(b) \
	do { \
        sram_bulk_write(b);\
        sram_bulk_write_next();\
        out_addr++;\
	} while(0)

		/*
		** Handle first byte separately (since we have to get angry
		** in case of an orphaned RLE code).
		*/
	INBYTE(in_byte);

	if (in_byte == RUNCHAR) {
		INBYTE(in_repeat);
		if (in_repeat != 0) {
			/* Note Error, not Incomplete (which is at the end
			** of the string only). This is a programmer error.
			*/
			printf("Orphaned RLE code at start\n");
			return 1;
		}
		OUTBYTE(RUNCHAR);
	} else {
		OUTBYTE(in_byte);
	}

	while( in_len > 0 ) {
		INBYTE(in_byte);
        if (in_len%1024==0)
            printf(".");
		if (in_byte == RUNCHAR) {
			INBYTE(in_repeat);
			if ( in_repeat == 0 ) {
				/* Just an escaped RUNCHAR value */
				OUTBYTE(RUNCHAR);
			} else {
				/* Pick up value and output a sequence of it */
                in_byte = last_byte; //;out_data[-1];
				while ( --in_repeat > 0 )
					OUTBYTE(in_byte);
			}
		} else {
			/* Normal byte */
			OUTBYTE(in_byte);
		}
        last_byte = in_byte;
	}
    sram_bulk_write_end();
	return 0;
}

