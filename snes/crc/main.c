#include "data.h";
#include "pad.h";
#include "event.h";
#include "myEvents.h";
#include "ressource.h";
#include "PPU.h"
#include "debug.h"
#include "crc.h"


#include <stdlib.h>

padStatus pad1;

void initInternalRegisters(void) {
	characterLocation[0] = 0x0000;
	characterLocation[1] = 0x0000;
	characterLocation[2] = 0x0000;
	characterLocation[3] = 0x0000;
	initDebugMap();
}

void preInit(void) {
	// For testing purpose ... 
	// Insert code here to be executed before register init
}

void main(void) {
	word i,j,y;
	word crc01;
	word crc02;
	padStatus pad1;
	char line_header[32] = "BANK CRC  ADDR   123456789ABCDEF";
    char line[32] = "                             ";	
	char test_buffer[] = "da";
    unsigned long addr; 
	initInternalRegisters();

	*(byte*) 0x2105 = 0x01;	// MODE 1 value
	*(byte*) 0x212c = 0x01; // Plane 0 (bit one) enable register
	*(byte*) 0x212d = 0x00;	// All subPlane disable
	*(byte*) 0x2100 = 0x0f; // enable background

    enableDebugScreen();

    writeln(line_header,0);

	while(1){
		addr = 0x008000;
		crc02 = crc_update(test_buffer,2);
        y = 0;
		for(j=0; j<32; j++) {
    		crc01 = crc_update_mem(addr,0x8000);
			int2hex((unsigned long)j,&line[0],4);
			int2hex((unsigned long)crc01,&line[5],4);
			int2hex((unsigned long)addr,&line[10],6);
            writeln(line,y + 1);
            y++;
            if (y==16)
                y = 0;
    		
    		addr += 0x010000;
		}
		while(!pad1.start) {
			waitForVBlank();
			pad1 = readPad((byte) 0);
		}
	}	
	while(1);
}

void IRQHandler(void) {
}

void NMIHandler(void) {
	//processEvents();
}
