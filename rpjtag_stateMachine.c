/*
 * Raspberry Pi JTAG Programmer using GPIO connector
 * Version 0.2 Copyright 2013 Rune 'Krelian' Joergensen
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

void UpdateState(int j_state)
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
		case JTAG_DR_SCAN: //Goto DR-SCAN
			break;
		case JTAG_IR_SCAN: //Goto IR-SCAN
			break;
		default:
			break;
	}
	//fprintf(stderr,"JTAG_STATE: %03X\n", jtag_state);
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
	UpdateState(JTAG_DR_SHIFT);
	send_cmd(0,0);
	send_cmd(0,0);
}

//Can only be used after sync
void SelectShiftIR()
{
	send_cmd(0,1);
	send_cmd(0,0);
	send_cmd(0,0);
}

//Can only be used after sync
void ExitShift()
{
	send_cmd(0,1);
	send_cmd(0,1);
	send_cmd(0,1);
}