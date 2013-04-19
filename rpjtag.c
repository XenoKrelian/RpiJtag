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
#include "rpjtag_io.h" //Basic IO functions

int CountDevices()
{
	if((parms & 0x01) == 0x01)
		fprintf(stderr,"Counting devices\n");

	IRTotalRegSize = 0;
	syncJTAGs();
	SelectShiftIR();

	for(x=0;x<MAX_CHAIN;x++) send_cmd(1,0); //Flush IR Registers, makes sure counting IR-Reg Size is currect
	
	for(i=0;i<MAX_CHAIN;i++)
 	{
		x = send_cmd(0,0);
		if(!x) break;
		IRTotalRegSize++;
	}

	for(x=0;x<MAX_CHAIN;x++) send_cmd(1,0); //PUT all in BYPASS
	
	ExitShift(); //Shift-IR (TMS)-> Exit1-IR (TMS) -> Update-IR (TMS) -> Select DR-Scan
	SelectShiftDR();

	for(x=0;x<MAX_CHAIN;x++) send_cmd(0,0); //Flush DR register, used during debug

	if((parms & 0x01) == 0x01)
		fprintf(stderr,"FLUSH DR\n");

	for(x=0;x<IRTotalRegSize;x++) //put in 1's into DR, when TDO outputs a 1, end is reached
	{
		i = send_cmd(1,0);
		if(i) break;
	}

	if(x==IRTotalRegSize) x = 0; //if x reaches max most likely a error ocurred!

	if((parms & 0x01) == 0x01)
		fprintf(stderr,"(IR REG SIZE:(0's) %d vs (1's) %d)\n",i,IRTotalRegSize);

	fprintf(stderr,"Devices found: %d\n",x);
	ExitShift();
	return x;
}

//Raspberry PI side.
void help()
{
	fprintf(stderr,
	"Usage: rpjtag [options]\n"
	"       -h          print help\n"
	"	-D	    Debug info\n"
	"\n");
}

//
// Set up a memory regions to access GPIO
//
void setup_io()
{
	/* open /dev/mem */
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
		printf("can't open /dev/mem \n");
	exit(-1);
	}
	
	/* mmap GPIO */
	gpio_map = mmap(
		NULL,             //Any adddress in our space will do
		BLOCK_SIZE,       //Map length
		PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
		MAP_SHARED,       //Shared with other processes
		mem_fd,           //File to map
	GPIO_BASE);

	   close(mem_fd); //No need to keep mem_fd open after mmap
	
	   if (gpio_map == MAP_FAILED) {
	      printf("mmap error %d\n", (int)gpio_map);
	      exit(-1);
	   }
	
	   // Always use volatile pointer!
	   gpio = (volatile unsigned *)gpio_map;
	
	INP_GPIO(JTAG_TCK);
	INP_GPIO(JTAG_TMS);
	INP_GPIO(JTAG_TDI);
	INP_GPIO(JTAG_TDO); //Receive output from Device to rpi

	nop_sleep(WAIT);
	if((parms & 0x01) == 0x01)
		fprintf(stderr,"INITIAL PORT STATUS TMS %d, TCK %d, TDI %d, TDO %d\n",GPIO_READ(JTAG_TMS),GPIO_READ(JTAG_TCK),GPIO_READ(JTAG_TDI),GPIO_READ(JTAG_TDO));	
	
	OUT_GPIO(JTAG_TDI); //Send data from rpi to Device
	OUT_GPIO(JTAG_TMS);
	OUT_GPIO(JTAG_TCK);

	if((parms & 0x01) == 0x01)
		fprintf(stderr,"READY PORT STATUS TMS %d, TCK %d, TDI %d, TDO %d\n",GPIO_READ(JTAG_TMS),GPIO_READ(JTAG_TCK),GPIO_READ(JTAG_TDI),GPIO_READ(JTAG_TDO));	


	nop_sleep(WAIT);
}

int main(int argc, char *argv[])
{
	char *ifile  = "demo.bit";
	//char *ofile = "jtag.hex";
	int opt;
	nDevices = 0;
	parms = 0;
	jtag_state = 0x00;

	fprintf(stderr, "Raspberry Pi JTAG Programmer, v0.1 (April 2013)\n\n");

	while ((opt = getopt(argc, argv, "hDi:")) != -1) {
		switch (opt) {
		case 'h':
			help();
			break;
		case 'D':
			parms |= 0x01;
			break;
		case 'i':
			ifile = optarg;
			break;
		default:
			fprintf(stderr, "\n");
			help();
			exit(0);
			break;
		}
	}

	#ifdef DEBUG
		parms |= 0x01;
	#endif

	FILE* bitstream = load_bit_file(ifile);

	// Set up gpi pointer for direct register access
	setup_io();

	syncJTAGs(); //Puts all JTAG Devices in RESET MODE
	nDevices = CountDevices(); //Count Number of devices, also finds total IR size
	readIDCODES(); //Reads the IDCODE's from Devices
	fprintf(stderr,"\n");
	if(nDevices != 0)
	{
		ProgramDevice(1,bitstream);
	}
	fclose(bitstream);
	munmap(gpio_map,BLOCK_SIZE);

	printf("\nDone\n");
	exit(0);
}