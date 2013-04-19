//Header file for rpjtag.c
//Credit for GPIO setup goes to http://elinux.org/RPi_Low-level_peripherals

//My Devices in dev/test circuit
#define PROM_XCF01S	0xf5044093
#define FPGA_XC3S200_FT256	0x01414093

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

#define MAX_CHAIN 199

int  mem_fd;
void *gpio_map;

// I/O access
volatile unsigned *gpio;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET(g) *(gpio+7) = 1<<(g) // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR(g) *(gpio+10) = 1<<(g) // clears bits which are 1 ignores bits which are 0
#define GPIO_READ(g) (*(gpio+13) >> (g)) & 0x00000001
#define GPIO_READRAW *(gpio+13)

//Perspective is from Device connected, so TDO is output from device to input into rpi
#define JTAG_TMS 9 //MISO 	PI ---> JTAG
#define JTAG_TDI 7 //CE1 	PI ---> JTAG
#define JTAG_TDO 8 //CE0 	PI <--- JTAG
#define JTAG_TCK 4 //#4 	PI ---> JTAG

//-D DEBUG when compiling, will make all sleeps last 0.5 second, this can be used to test with LED on ports, or pushbuttons
//Else sleeps can be reduced to increase speed
#ifdef DEBUG
#define WAIT 500000000 //aprox 0.5s
#else
#define WAIT 125 //aprox 0.5us
#endif

//For State Machine
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

#define XILINX_BITFILE 1

int n, x, i, temp, IRTotalRegSize, jtag_state, nDevices, parms, bitFileId;

struct IDCODE_DATA
{
	int onebit:1;
	int manuf:11;
	int size:9;
	int family:7;
	int rev:1;
} idcode[32]; //32 Devices

/*
Devices:
 IDCODE  Manuf     Device name                               len hir tir hdr tdr
0141C093 Xilinx    XC3S400                                    6   8   0   1   0
F5045093 Xilinx    XCF02S                                     8   0   6   0   1

BSR chain is: pre(0/0) XCF02S(8/25) mid(0/0) XC3S400(6/815) end(0/0) => 14/840
Copied 815 cells from XC3S400 to safe bits to postion 25
Copied 25 cells from XCF02S to safe bits to postion 0
di  : rd=686 wr=685 con=684 dis=1
do  : rd=650 wr=649 con=648 dis=1
sclk: rd=689 wr=688 con=687 dis=1
sel : rd=641 wr=640 con=639 dis=1
wp  : rd=683 wr=682 con=681 dis=1
hold: rd=692 wr=691 con=690 dis=1
Copied 815 cells from XC3S400 to safe bits to postion 25
Copied 25 cells from XCF02S to safe bits to postion 0
*/

struct Device
{
	struct Device *nextDevice; //Pointer to next Device
	char* DeviceName; //Device name
	int idcode:32; //Device Id Code
	int tIRLen; //Total IR Lenght
	int tDRLen; //Total DR Lenght
	int dIRLen; //Device IR Lenght
	int dDRLen; //Device DR Lenght
};

struct bitFileInfo
{
	int BitFile_Type;
	char* DesignName;
	char* DeviceName;
	int Bitstream_Length;
};

struct bitFileInfo bitfileinfo;

/*
struct Device {
	struct Device *next;

	unsigned int id; //long
	int hir, tir, hdr, tdr, len;
	int bsrlen, bsrsample, bsrsafe;
	int user;

	char *manuf, *name;
};
*/

void ProgramDevice(int deviceNr, FILE *f);
void send_byte(unsigned char byte, int lastByte);
FILE* load_bit_file(char *ifile);
void CreateDevice(struct IDCODE_DATA newDevice);
int GetSegmentLength(int segment, int segmentCheck, FILE *f);
void parse_header();
void UpdateState(int j_state);
void nop_sleep(long x);
int read_jtag_tdo();
int send_cmd(int iTDI,int iTMS);
void send_cmdWord_msb_first(unsigned int cmd, int lastBit, int bitoffset);
void send_cmdWord_msb_last(unsigned int cmd, int lastBit, int bitoffset);
void syncJTAGs();
void SelectShiftDR();
void SelectShiftIR();
void ExitShift();
int CountDevices();
void readIDCODES();
void jtag_read_data(char* data,int iSize);
void checkUserId(int x);
void help();
void setup_io();
