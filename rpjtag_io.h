int read_jtag_tdo()
{
	return ( GPIO_READ(JTAG_TDO) ) ? 1 : 0;
}

int send_cmd(int iTDI,int iTMS)
{
	
	int tdo = read_jtag_tdo();

	if(iTDI == 1)
		GPIO_SET(JTAG_TDI);
	else
		GPIO_CLR(JTAG_TDI);

	if(iTMS == 1)
	{
		GPIO_SET(JTAG_TMS);
	}
	else
		GPIO_CLR(JTAG_TMS);

	UpdateState(JTAG_AUTO,iTMS); //Auto adjust statemachine

	nop_sleep(WAIT);
	GPIO_SET(JTAG_TCK);
	nop_sleep(WAIT);
	GPIO_CLR(JTAG_TCK);
	nop_sleep(WAIT);
	return tdo;
}

//Mainly used for command words (CFG_IN)
void send_cmdWord_msb_first(unsigned int cmd, int lastBit, int bitoffset) //Send data, example 0xFFFF,0,20 would send 20 1's, with not TMS
{
	while(bitoffset--)
	{
		int x = ( cmd >> bitoffset) & 0x01;
		send_cmd(x,(lastBit==1 && bitoffset==0));
	}
}

//Mainly used for IR Register codes
void send_cmdWord_msb_last(unsigned int cmd, int lastBit, int bitoffset) //Send data, example 0xFFFF,0,20 would send 20 1's, with not TMS
{
	int i;
	for(i=0;i<bitoffset;i++)
	{
		int x = ( cmd >> i ) & 0x01;
		send_cmd(x,(lastBit==1 && bitoffset==i+1));
	}
}

void send_byte(unsigned char byte, int lastByte) //Send single byte, example from fgetc
{
	int x;
	for(x=7;x>=0;x--)
	{
		send_cmd(byte>>x&0x01,(lastByte==1 && x==0));
	}
}

//Does a NOP call in BCM2708, and is meant to be run @ 750 MHz
void nop_sleep(long x)
{
	while (x--) {
		asm("nop");
	}
}

void jtag_read_data(char* data,int iSize)
{
	if(iSize==0) return;
	int bitOffset = 0;
	memset(data,0,(iSize+7)/8);

	iSize--;
	while(iSize--)
	{
		temp = send_cmd(0,0);
		data[bitOffset/8] |= (temp << (bitOffset & 7));
		bitOffset++;
	}

	temp = send_cmd(0,1); //Read last bit, while also going to EXIT
	data[bitOffset/8] |= (temp << (bitOffset & 7));
	send_cmd(0,1); //Go to UPDATE STATE
	send_cmd(0,1); //Go to SELECT DR-SCAN
}

/*
	This reads IDCODE from Devices in JTAG, when IDCODE is found that matches Bitfile+BDSL file, device position in JTAG chain is stored
	Future fix:
		Able to program all/specific device if more then 1 device of same type
		example:
		PROM->XC3S200->PROM->XC3S200
		only the last one(XC3S200) will be found by the code below
*/
void readIDCODES()
{
	if(nDevices != 0 || ((parms & 0x01) == 0x01))
	{
		syncJTAGs();
		send_cmd(0,0);
		send_cmd(0,0);
		jtag_read_data((char*)idcode,32*nDevices);
		//Remember device are reserved order, first data out is from last device, hence it will be device 1, need to fix this somehow to avoid confustion
		x = 0;
		for(i=0;i<nDevices;i++)
		{
			if(idcode[i].onebit == 1 || idcode[i].onebit == -1) //bug with 1st bit in readout, should always be 1, but this gets -1, need help to fix
			{
				int iIdcode = (*(int*)&idcode[i]|0xF0000000);
				if(iIdcode == device_data[deviceDataNr].idcode && x == 0)
				{
					x=1;
					deviceChainNr = i;
					fprintf(stderr,"\nDevice %d, %08X (Manuf %03X, Part size %03X, Family code %02X) bitfile+bdsl match: %d",i+1,iIdcode,idcode[i].manuf,idcode[i].size,idcode[i].family,x);
				}else{
					fprintf(stderr,"\nDevice %d, %08X (Manuf %03X, Part size %03X, Family code %02X) bitfile+bdsl match: %d",i+1,iIdcode,idcode[i].manuf,idcode[i].size,idcode[i].family,0);
				}
			}
		}
		if(x==0){fprintf(stderr,"ERROR NO DEVICE IDCODE MATCH BITFILE/BDSL FILE"); exit(1);}
		
	}else{
		fprintf(stderr,"No devices found\n");
	}
}