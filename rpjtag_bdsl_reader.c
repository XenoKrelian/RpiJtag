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

/*Can properly be optimized even more
So far only finds DeviceName, IRLen, IDCODE and Boundary lenght
*/
void read_bdsl_file(char *filename, int fileNr)
{
	if((parms & 0x01) == 0x01) fprintf(stderr,"\nBDSL file: %s\n",filename);
	FILE* f;
	f = fopen(filename, "r");
	int fSize, result;
	if(!f)
	{
		fprintf(stderr, "RpiJtag: could not open BDSL file (%s) for read, aborting.\n", filename); 
		exit(1);
	}else{
		if((parms & 0x01) == 0x01) fprintf(stderr, "RpiJtag: BDSL file (%s) has been read, continuing.\n", filename); 
	}

	fseek(f , 0 , SEEK_END);
	fSize = ftell(f);
	rewind(f);

	char* buffer = (char*) malloc (sizeof(char)*fSize);
	if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}
	result = fread(buffer,1,fSize,f);
	if (result != fSize) {fputs ("Reading error",stderr); exit (3);}

	device_data[fileNr].filePtr = f;
	fclose(f); //Remove this

	int lineNr = 1;
	int skipLine = 0;
	int newLine = 0;
	char c;
	char DeviceName[40]; //Max 39 char in devicename + '\0'
	char DeviceIDCODE[33]; //Holds string representation of IDCODE, 33rd bit is '\0'
	int iDeviceIDCODE;
	int irLen=0;
	int bsr=0;
	DeviceName[0] = '\0';
	DeviceIDCODE[0] = '\0';

	for(n=0;n<fSize;n++)
	{
		c = buffer[n];
		if((c == 0x0D && buffer[n+1] == 0x0A)) //if newline char, increase linenr value
		{
			lineNr++;
			n++; 
			skipLine = 0;
			newLine = 1;
		}
		else if(c==0x2D && buffer[n+1] == 0x2D)
		{
			skipLine = 1;
			n++;
		}else{
			if(newLine && !skipLine)
			{
				//Gets Entity name for rest of file, if this is not a start of file, the rest most likely fails
				if(c =='e' && buffer[n+1] == 'n' && buffer[n+2] == 't' && buffer[n+3] == 'i' && buffer[n+4] == 't' && buffer[n+5] == 'y')
				{
					n += 7; //Skips whitespace / 0x20
					i=0;
					while(1)
					{
						if(buffer[i+n] == 0x20)
						{
							DeviceName[i] = '\0';
							break;
						}
						else
						{
							DeviceName[i] = buffer[i+n];
						}
						i++;
					}

				}
				else if(strlen(DeviceName)!=0)
				{
					if(buffer[n] == 'a' && memcmp(buffer+n,"attribute INSTRUCTION_LENGTH of ",32) == 0)
					{
						int IrLenStart = 0;
						int IrLenEnd = 0;
						n+=32; //skip found string
						n+=strlen(DeviceName); //Skip Device name

						IrLenStart = strcspn (buffer+n,"1234567890"); //find first number, which marks start of length
						IrLenEnd = strcspn (buffer+n,";"); //find end if attribute

						irLen = strtoul(buffer+n+IrLenStart,NULL,10); //reads all numbers until non-numeric character is found
						if((parms & 0x01) == 0x01) fprintf (stderr,"%s IRLEN: %d.\n",DeviceName, irLen);
						device_data[fileNr].dIRLen = irLen;
						IRTotalRegSize += irLen;
						n+=IrLenEnd; //Makes n continue after our found value;
					}
					else if(buffer[n] == 'a' && memcmp(buffer+n,"attribute BOUNDARY_LENGTH of ",29) == 0)
					{
						int BoundaryLenStart = 0;
						int BoundaryLenEnd = 0;
						n+=29; //skip found string
						n+=strlen(DeviceName); //Skip Device name

						BoundaryLenStart = strcspn (buffer+n,"1234567890"); //find first number, which marks start of length
						BoundaryLenEnd = strcspn (buffer+n,";"); //find end if attribute

						bsr = strtoul(buffer+n+BoundaryLenStart,NULL,10); //reads all numbers until non-numeric character is found
						if((parms & 0x01) == 0x01) fprintf (stderr,"%s BSR LEN: %d.\n",DeviceName, bsr);
						device_data[fileNr].dBSRLen = bsr;
						n+=BoundaryLenEnd; //Makes n continue after our found value;
					}
					else if(buffer[n] == 'a' && memcmp(buffer+n,"attribute IDCODE_REGISTER of ",29) == 0)
					{
						int IDCODEStart = 0;
						int IDCODEPos = 0;
						int IDCODEEnd = 0;
						n+=29; //skip found string
						n+=strlen(DeviceName); //Skip Device name*/

						/*
						IDCODE is usally split into segment, inside ""
						*/
						IDCODEEnd = n + strcspn(buffer+n,";");
						IDCODEStart = strcspn(buffer+n,"\"");
						n += IDCODEStart + 1;
						i=0;
						while(n<IDCODEEnd)
						{
							if(buffer[n] == 0x22)
							{
								n++;//Skip "
								IDCODEPos = strcspn(buffer+n,"\"");
								n+=IDCODEPos; //Next n++ will skip " too
							}
							else
							{
								if(buffer[n]=='X')
									DeviceIDCODE[i]=0x31; //1
								else
									DeviceIDCODE[i]=buffer[n];
								i++;
							}
							n++;
						}
						DeviceIDCODE[i] = '\0';
						iDeviceIDCODE = strtoul(DeviceIDCODE,NULL,2);
						device_data[fileNr].idcode = iDeviceIDCODE;
						if((parms & 0x01) == 0x01) fprintf (stderr,"%s IDCODE %X\n",DeviceName, iDeviceIDCODE);
					}
				}
			}

			if(bsr > 0 && irLen > 0 && strlen(DeviceName) > 0)
				break;

			newLine = 0;
		}
	}

	//Remove specials char and convert to lower case (-32), bitfile only uses lowercase with no specials char
	i=0;
	for(n=0;n<strlen(DeviceName);n++)
	{
		if(DeviceName[n] >=65 && DeviceName[n] <=90) //convert all UPPERCASE to lowercase
		{
			DeviceName[i] = (DeviceName[n]+32);
			i++;
		}
		else if(DeviceName[n] >=48 && DeviceName[n] <=57) //Move number
		{
			DeviceName[i] = DeviceName[n];
			i++;
		}
	}
	DeviceName[i] = '\0';
	device_data[fileNr].DeviceName = malloc(strlen(DeviceName)*sizeof(char));
	memcpy(device_data[fileNr].DeviceName,DeviceName,strlen(DeviceName)+1);
	free(buffer);
}

void load_bdsl_files(char *bdslfiles)
{
	//tried using strlok, but got segmentation error
	int strlength = strlen(bdslfiles);
	char *temp = NULL;
	char *temp2 = NULL;
	int wordCounter = 0;
    for(x=0;x<strlength;x++)
	{
		if(bdslfiles[x] == ',' || x==strlength)
		{
			read_bdsl_file(temp,fileCounter);
			fileCounter++; 
			wordCounter = 0;
			temp=NULL;
		}else{
			temp2 = (char*)realloc(temp,(wordCounter+1)*sizeof(char));
			if(temp2!=NULL)
			{
				temp=temp2;
				temp[wordCounter] = bdslfiles[x];
			}else{
				free(temp);
				fprintf(stderr,"Error (re)allocating memory");	
			}
			wordCounter++;
		}
	}
	read_bdsl_file(temp,fileCounter);
	free(temp); //Clean-up
	//reset global defined variables
	x=0;
	i=0;
	n=0;
}