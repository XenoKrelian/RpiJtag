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

FILE* load_bit_file(char *ifile)
{
	FILE *temp;
	temp = fopen(ifile, "r");
	if(!temp)
	{
		fprintf(stderr, "rpjtag: could not open bitstream file %s for read, aborting.\n", ifile); 
		exit(1);
	}else{
		fprintf(stderr, "rpjtag: bitstream file %s has been read, continuing.\n", ifile); 
	}
	parse_header(temp);
	return temp;
}

void checkStatusReg()
{
	//Xilinx CFG_IN Command words
	unsigned int DYMMYWORD = 0xFFFFFFFF;
	unsigned int NOOP = 0x20000000;
	unsigned int SYNCWORD = 0xAA995566;
	unsigned int STATUS_REG = 0x2800E002;
	
	syncJTAGs();
	send_cmd(0,1); //SELECT IR
	send_cmd(0,0);
	send_cmd(0,0);

	//CFG_IN
	send_cmdWord_msb_last(0x3FC5,1,14); //11111111 000101

	send_cmd(0,1); // -> UPDATE-IR
	send_cmd(0,1); // -> SELECT-DR SCAN
	send_cmd(0,0);
	send_cmd(0,0);

	for(n=0;n<31;n++) send_cmd(0,0); //31 Leading Zero's

	send_cmdWord_msb_first(DYMMYWORD,0,32);
	send_cmdWord_msb_first(SYNCWORD,0,32);
	send_cmdWord_msb_first(NOOP,0,32);
	send_cmdWord_msb_first(STATUS_REG,0,32);
	send_cmdWord_msb_first(NOOP,0,32);
	send_cmdWord_msb_first(NOOP,1,32);

	for(n=0;n<3;n++) send_cmd(0,1); //Shift-Ir
	send_cmd(0,0);
	send_cmd(0,0);

	//CFG_OUT
	send_cmdWord_msb_last(0x3FC4,1,14); //11111111 000100

	send_cmd(0,1);
	send_cmd(0,1); //SELECT-DR
	send_cmd(0,0);
	send_cmd(0,0); //SHIFT-DR

	fprintf(stderr,"\nSTAT: ");
	for(n=0;n<32;n++)
	{
			fprintf(stderr,"%d",send_cmd(0,0));
	}
	fprintf(stderr,"\n");

	UpdateState(JTAG_RESET);
	for(n=0;n<5;n++) send_cmd(0,1);
}

void ProgramDevice(int deviceNr, FILE *f)
{
	unsigned char tempChar;

	if((parms & 0x01) == 0x01)
		checkUserId(0);

	checkStatusReg();

	syncJTAGs();
	//UpdateState(JTAG_IR_SHIFT);
	send_cmd(0,1); // -> SELECT IR SCAN
	send_cmd(0,0);
	send_cmd(0,0);

	//CFG_IN
	send_cmdWord_msb_last(0x3FC5,1,14); //11111111 000100
	//FPGA in CFG_IN MODE and PROM in BYPASS

	send_cmd(0,1); //->UPDATE IR
	send_cmd(0,1); //SELECT SCAN-DR
	send_cmd(0,0);
	send_cmd(0,0);

	for(n=0;n<31;n++) send_cmd(0,0); //31 Leading Zero's

	int n,bitcount = 0;
	fprintf(stderr, "\nProgramming ... (%d) ",bitfileinfo.Bitstream_Length);
	for(n = 0; n < bitfileinfo.Bitstream_Length; n++)
	{
		tempChar = fgetc(f);
		if(n < 4 && tempChar != 0xFF)
		{
			fprintf(stderr, "\nError in bitfile, no Dummy Word at start");
			exit(1);
		}
		send_byte(tempChar, ( n==(bitfileinfo.Bitstream_Length-1) ) );
		if((bitcount % (256 * 1024)) == 0) putc('*', stderr);
		bitcount += 8;
	}
	//Last bit has TMS 1, so we are in EXIT-DR
	fprintf(stderr, "\nProgrammed %d bits, n==%d\n", bitcount,n);
	send_cmd(0,1); // -> UPDATE-DR
	send_cmd(0,1); // -> SELECT DR SCAN
	send_cmd(0,1); // -> SELECT IR SCAN
	send_cmd(0,0);
	send_cmd(0,0);

	//JSTART
	send_cmdWord_msb_last(0x3FCC,1,14); //11111111 000100
	//FPGA in CFG_IN MODE and PROM in BYPASS

	send_cmd(0,1); //->UPDATE IR
	for(n=0;n<16;n++) send_cmd(0,0); //Go to Run-Test, and idle for 12+ clocks to start

	checkStatusReg();

	if((parms & 0x01) == 0x01)
	{
		checkUserId(1);
		for(n=0;n<5;n++) send_cmd(0,1);
		send_cmd(0,0);
	}
}

void checkUserId(int x)
{
	syncJTAGs();
	send_cmd(0,1); //SELECT IR
	send_cmd(0,0);
	send_cmd(0,0);

	send_cmd(0,0); //LSB in FPGA
	send_cmd(0,0);
	send_cmd(0,0);
	send_cmd(1,0);
	send_cmd(0,0);
	send_cmd(0,0); //MSB in FPGA

	send_cmd(1,0); //LSB in PROM
	send_cmd(1,0);
	send_cmd(1,0);
	send_cmd(1,0);
	send_cmd(1,0);
	send_cmd(1,0);
	send_cmd(1,0);
	send_cmd(1,1); //MSB in PROM -> EXIT IR
	send_cmd(0,1);
	send_cmd(0,1);
	send_cmd(0,0);
	send_cmd(0,0);

	int data;
	jtag_read_data((char *)&data,32);
	if(!x)
		fprintf(stderr,"\nUserID Before programming: %08X",data);
	else
		fprintf(stderr,"\nUserID After programming: %08X",data);
	send_cmd(0,1);
	send_cmd(0,0);
	send_cmd(0,0); //Run-Test/Idle
}

void CreateDevice(struct IDCODE_DATA newDevice)
{

}

int GetSegmentLength(int segment, int segmentCheck, FILE *f)
{
	//Get Next Segment
	segment = fgetc(f);
	if(segment != segmentCheck)
	{
		fprintf(stderr, "Error in header segment: %d, should be %d\n",segment,segmentCheck);
		exit(1);
	}else if((parms & 0x01) == 0x01) {
		fprintf(stderr, "Segment: %c\n",(char)segment); //For Debuging
	}

	return ((fgetc(f) << 8) + fgetc(f)); //Lenght of segment
}

//Made from XC3S200_FT256 bit file
//Code is based on code from http://panteltje.com/panteltje/raspberri_pi/
//And David Sullins BitInfo/BitFile code
void parse_header(FILE *f)
{
	//00 09  0F F0  0F F0  0F F0  0F F0  00 00 01 
	unsigned int XilinxID13[] = {0x00,0x09, 0x0F,0xF0, 0x0F,0xF0, 0x0F,0xF0, 0x0F,0xF0, 0x00,0x00,0x01};
	unsigned int *HeaderBufferData;
	int segmentCheck = 0x61; //Start at a char(97)
	int segment = 0;
	int	segmentLength = 0;
	unsigned char tempChar;

	//First 13 bits, so far all Xilinx Bitstream files, start with XilinxID13 bytes
	//Seems to be identifier for bitfile creator
	int t;
	HeaderBufferData = (unsigned int*)malloc( sizeof(unsigned int) *13);
	for(t=0;t<13;t++) HeaderBufferData[t] = fgetc(f);

	if(0==memcmp(HeaderBufferData,XilinxID13,sizeof(HeaderBufferData)*13)) 
		bitFileId = XILINX_BITFILE;
	else
		bitFileId = 0;
	//if(0==memcmp(HeaderBufferData,XilinxID13,sizeof(HeaderBufferData)*13)) bitFileId = Altera_BITFILE; else bitFileId = 0; //Future stuff
	free(HeaderBufferData);

	fprintf(stderr, "Bitfile type: %d\n",bitFileId);
	bitfileinfo.BitFile_Type = bitFileId;
//----------------------------------------------------------------------
	segmentLength = GetSegmentLength(segment,segmentCheck,f);
	segmentCheck++;
	fprintf(stderr, "Design Name: ");
	bitfileinfo.DesignName = (char*)malloc(sizeof(char)*segmentLength);
	for(x = 0; x < segmentLength; x++)
	{
		bitfileinfo.DesignName[x] = fgetc(f);
	}
	fprintf(stderr, "%s\n",bitfileinfo.DesignName);
//----------------------------------------------------------------------
	segmentLength = GetSegmentLength(segment,segmentCheck,f);
	segmentCheck++;
	fprintf(stderr, "Device: ");
	bitfileinfo.DeviceName = (char*)malloc(sizeof(char)*segmentLength);
	for(x = 0; x < segmentLength; x++)
	{
		bitfileinfo.DeviceName[x] = fgetc(f);
	}
	fprintf(stderr, "%s\n",bitfileinfo.DeviceName);
//----------------------------------------------------------------------
	segmentLength = GetSegmentLength(segment,segmentCheck,f);
	segmentCheck++;
	fprintf(stderr, "Date: ");
	for(x = 0; x < segmentLength; x++)
		{
		tempChar = fgetc(f);
		fprintf(stderr, "%c", tempChar);
		}
	fprintf(stderr, "\n");
//----------------------------------------------------------------------
	segmentLength = GetSegmentLength(segment,segmentCheck,f);
	segmentCheck++;
	fprintf(stderr, "Time: ");
	for(x = 0; x < segmentLength; x++)
		{
		tempChar = fgetc(f);
		fprintf(stderr, "%c", tempChar);
		}
	fprintf(stderr, "\n");
//----------------------------------------------------------------------
	segment = fgetc(f);
	if(segment != segmentCheck)	fprintf(stderr, "Error in header segment: %d, should be %d\n",segment,segmentCheck);
	if((parms & 0x01) == 0x01)
		fprintf(stderr, "Segment: %c\n",(char)segment); //For Debuging
	segmentLength = (fgetc(f) << 24) + (fgetc(f) << 16) + (fgetc(f) << 8) + fgetc(f);
	fprintf(stderr, "Bitstream Length: %0d bits\n", segmentLength * 8);
	bitfileinfo.Bitstream_Length = segmentLength;

	//That's it for now, bitfile header info is stored in bitfileinfo, FILE *f, needs to be kept until we program
	//This leaves us at 0xFF FF FF FF (Dummy Word) and the Sync Cmd 0xAA 99 55 66 (XILINX)
}