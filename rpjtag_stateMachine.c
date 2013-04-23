/*
 * Raspberry Pi JTAG Programmer using GPIO connector
 * Version 0.3 Copyright 2013 Rune 'Krelian' Joergensen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "rpjtag.h" //Defines

/*
#define JTAG_AUTO			0x000 // 0000 0000 0000 //0
#define JTAG_RESET			0x001 // 0000 0000 0001 //1
#define JTAG_IDLE			0x002 // 0000 0000 0010 //2

#define JTAG_DR_SCAN		0x003 // 0000 0000 0011 //3
#define JTAG_DR_CAPTURE		0x011 // 0000 0001 0001 //17
#define JTAG_DR_SHIFT		0x012 // 0000 0001 0010 //18
#define JTAG_DR_EXIT1		0x013 // 0000 0001 0011 //19
#define JTAG_DR_PAUSE		0x014 // 0000 0001 0100 //20
#define JTAG_DR_EXIT2		0x015 // 0000 0001 0101 //21
#define JTAG_DR_UPDATE		0x016 // 0000 0001 0110 //22

#define JTAG_IR_SCAN		0x004 // 0000 0000 0100 //4
#define JTAG_IR_CAPTURE		0x101 // 0001 0000 0001 //257
#define JTAG_IR_SHIFT		0x102 // 0001 0000 0010 //258
#define JTAG_IR_EXIT1		0x103 // 0001 0000 0011 //259
#define JTAG_IR_PAUSE		0x104 // 0001 0000 0100 //260
#define JTAG_IR_EXIT2		0x105 // 0001 0000 0101 //261
#define JTAG_IR_UPDATE		0x106 // 0001 0000 0110 //262
*/

//Entire file needs a rewrite, maybe routing table, for easy of transition in TAP controller
void UpdateState(int j_state, int iTMS)
{
	switch(j_state)
	{
		case JTAG_RESET:
			GPIO_SET(JTAG_TMS);
			for(i=0;i<5;i++) //make sure we are in run-test/idle
			{
				nop_sleep(WAIT);
				GPIO_CLR(JTAG_TCK);
				nop_sleep(WAIT);
				GPIO_SET(JTAG_TCK);
				nop_sleep(WAIT);
			}
			GPIO_CLR(JTAG_TCK);
			nop_sleep(WAIT);
			jtag_state=JTAG_RESET;
			break;
		case JTAG_AUTO: //auto called when TMS is high, this one does no TMS=1,TCK=1
			if((jtag_state & 0xFFE) < 15)
			{
				jtag_state++;
				if(jtag_state > 5)
					jtag_state = JTAG_RESET;
			}
			else if((jtag_state | 0x116) == 0x116) //if we are in UPDATE-IR/
				jtag_state = JTAG_DR_SCAN;
			break;
	}
}

void syncJTAGs()
{
	GPIO_SET(JTAG_TMS);
	for(i=0;i<5;i++) //make sure we are in run-test/idle
	{
		nop_sleep(WAIT);
		GPIO_CLR(JTAG_TCK);
		nop_sleep(WAIT);
		GPIO_SET(JTAG_TCK);
		nop_sleep(WAIT);
	}
	GPIO_CLR(JTAG_TCK);
	nop_sleep(WAIT);
	
	send_cmd(0,0); //Idle clock
	send_cmd(0,1); //SELECT-DR-SCAN
	jtag_state = JTAG_DR_SCAN;
}

//Can only be used after sync
void SelectShiftDR()
{
	jtag_state = JTAG_DR_SHIFT;
	send_cmd(0,0);
	send_cmd(0,0);
}

//Can only be used after sync
void SelectShiftIR()
{
	jtag_state = JTAG_IR_SHIFT;
	send_cmd(0,1);
	send_cmd(0,0);
	send_cmd(0,0);
}

//Can only be used after sync, leaves TAP controller in JTAG_DR_SCAN
void ExitShift()
{
	jtag_state = JTAG_DR_SCAN;
	send_cmd(0,1);
	send_cmd(0,1);
	send_cmd(0,1);
}