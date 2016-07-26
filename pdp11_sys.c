/* pdp11_sys.c: PDP-11 simulator interface

   Copyright (c) 1993-1998,
   Robert M Supnik, Digital Equipment Corporation
   Commercial use prohibited

   30-Mar-98	RMS	Fixed bug in floating point display.
   12-Nov-97	RMS	Added bad block table routine.
*/

#include "pdp11_defs.h"
#include <ctype.h>

extern DEVICE cpu_dev;
extern DEVICE ptr_dev, ptp_dev;
extern DEVICE tti_dev, tto_dev;
extern DEVICE lpt_dev, clk_dev;
extern DEVICE rk_dev, rx_dev;
extern DEVICE rl_dev, rp_dev;
extern DEVICE tm_dev;
extern UNIT cpu_unit;
extern REG cpu_reg[];
extern unsigned int16 *M;
extern int32 saved_PC;

/* SCP data structures and interface routines

   sim_name		simulator name string
   sim_PC		pointer to saved PC register descriptor
   sim_emax		number of words for examine
   sim_devices		array of pointers to simulated devices
   sim_stop_messages	array of pointers to stop messages
   sim_load		binary loader
*/

char sim_name[] = "PDP-11";

REG *sim_PC = &cpu_reg[0];

int32 sim_emax = 4;

#ifdef GAMEBOY
const DEVICE *sim_devices[] = { &cpu_dev,
#else
DEVICE *sim_devices[] = { &cpu_dev,
#endif
	&ptr_dev, &ptp_dev, &tti_dev, &tto_dev,
	&lpt_dev, &clk_dev, &rk_dev, &rl_dev,
	&rp_dev, &rx_dev, &tm_dev, NULL };

const char *sim_stop_messages[] = {
	"Unknown error",
	"Red stack trap",
	"Odd address trap",
	"Memory management trap",
	"Non-existent memory trap",
	"Parity error trap",
	"Privilege trap",
	"Illegal instruction trap",
	"BPT trap",
	"IOT trap",
	"EMT trap",
	"TRAP trap",
	"Trace trap",
	"Yellow stack trap",
	"Powerfail trap",
	"Floating point exception",
	"HALT instruction",
	"Breakpoint",
	"Wait state",
	"Trap vector fetch abort",
	"Trap stack push abort"  };

/* Binary loader.

   Loader format consists of blocks, optionally preceded, separated, and
   followed by zeroes.  Each block consists of:

	001		---
	xxx		 |
	lo_count	 |
	hi_count	 |
	lo_origin	 > count bytes
	hi_origin	 |
	data byte	 |
	:		 |
	data byte	---
	checksum

   If the byte count is exactly six, the block is the last on the tape, and
   there is no checksum.  If the origin is not 000001, then the origin is
   the PC at which to start the program.
*/

t_stat sim_load (FILE *fileref)
{
#ifdef GAMEBOY
int32 origin = 0, csum, count = 0, state, i;
#else
int32 origin, csum, count, state, i;
#endif

state = csum = 0;
while ((i = getc (fileref)) != EOF) {
	csum = csum + i;				/* add into chksum */
	switch (state) {
	case 0:						/* leader */
		if (i == 1) state = 1;
		else csum = 0;
		break;
	case 1:						/* ignore after 001 */
		state = 2;
		break;
	case 2:						/* low count */
		count = i;
		state = 3;
		break;
	case 3:						/* high count */
		count = (i << 8) | count;
		state = 4;
		break;
	case 4:						/* low origin */
		origin = i;
		state = 5;
		break;
	case 5:						/* high origin */
		origin = (i << 8) | origin;
		if (count == 6) {
			if (origin != 1) saved_PC = origin & 0177776;
			return SCPE_OK;  }
		count = count - 6;
		state = 6;
		break;
	case 6:						/* data */
		if (origin >= MEMSIZE) return SCPE_NXM;
		M[origin >> 1] = (origin & 1)?
			(M[origin >> 1] & 0377) | (i << 8):
			(M[origin >> 1] & 0177400) | i;
		origin = origin + 1;
		count = count - 1;
		state = state + (count == 0);
		break;
	case 7:						/* checksum */
		if (csum & 0377) return SCPE_CSUM;
		csum = state = 0;
		break;  }				/* end switch */
	}						/* end while */
return SCPE_FMT;					/* unexpected eof */
}

/* Factory bad block table creation routine

   This routine writes a DEC standard 044 compliant bad block table on the
   last track of the specified unit.  The bad block table consists of 10
   repetitions of the same table, formatted as follows:

	words 0-1	pack id number
	words 2-3	cylinder/sector/surface specifications
	 :
	words n-n+1	end of table (-1,-1)

   Inputs:
	uptr	=	pointer to unit
	sec	=	number of sectors per surface
	wds	=	number of words per sector
   Outputs:
	sta	=	status code
*/

t_stat pdp11_bad_block (UNIT *uptr, int32 sec, int32 wds)
{
int32 i, da;
int16 *buf;

if ((sec < 2) || (wds < 16)) return SCPE_ARG;
if ((uptr -> flags & UNIT_ATT) == 0) return SCPE_UNATT;
if (!get_yn ("Overwrite last track? [N]", FALSE)) return SCPE_OK;
da = (uptr -> capac - (sec * wds)) * sizeof (int16);
if (fseek (uptr -> fileref, da, SEEK_SET)) return SCPE_IOERR;
if ((buf = malloc (wds * sizeof (int16))) == NULL) return SCPE_MEM;
buf[0] = buf[1] = 012345u;
buf[2] = buf[3] = 0;
for (i = 4; i < wds; i++) buf[i] = 0177777u;
for (i = 0; (i < sec) && (i < 10); i++)
	fxwrite (buf, wds, sizeof (int16), uptr -> fileref);
free (buf);
if (ferror (uptr -> fileref)) return SCPE_IOERR;
return SCPE_OK;
}

/* Symbol tables */

#define I_V_L		16				/* long mode */
#define I_V_D		17				/* double mode */
#define I_L		(1 << I_V_L)
#define I_D		(1 << I_V_D)

/* Warning: for literals, the class number MUST equal the field width!! */

#define I_V_CL		18				/* class bits */
#define I_M_CL		017				/* class mask */
#define I_V_NPN		0				/* no operands */
#define I_V_REG		1				/* reg */
#define I_V_SOP		2				/* operand */
#define I_V_3B		3				/* 3b literal */
#define I_V_FOP		4				/* flt operand */
#define I_V_AFOP	5				/* fac, flt operand */
#define I_V_6B		6				/* 6b literal */
#define I_V_BR		7				/* cond branch */
#define I_V_8B		8				/* 8b literal */
#define I_V_SOB		9				/* reg, disp */
#define I_V_RSOP	10				/* reg, operand */
#define I_V_ASOP	11				/* fac, operand */
#define I_V_ASMD	12				/* fac, moded int op */
#define I_V_DOP		13				/* double operand */
#define I_V_CCC		14				/* CC clear */
#define I_V_CCS		15				/* CC set */
#define I_NPN		(I_V_NPN << I_V_CL)
#define I_REG		(I_V_REG << I_V_CL)
#define I_3B		(I_V_3B << I_V_CL)
#define I_SOP		(I_V_SOP << I_V_CL)
#define I_FOP		(I_V_FOP << I_V_CL)
#define I_6B		(I_V_6B << I_V_CL)
#define I_BR		(I_V_BR << I_V_CL)
#define I_8B		(I_V_8B << I_V_CL)
#define I_AFOP		(I_V_AFOP << I_V_CL)
#define I_ASOP		(I_V_ASOP << I_V_CL)
#define I_RSOP		(I_V_RSOP << I_V_CL)
#define I_SOB		(I_V_SOB << I_V_CL)
#define I_ASMD		(I_V_ASMD << I_V_CL)
#define I_DOP		(I_V_DOP << I_V_CL)
#define I_CCC		(I_V_CCC << I_V_CL)
#define I_CCS		(I_V_CCS << I_V_CL)

GAMEBOY_STATIC const int32 masks[] = {
0177777, 0177770, 0177700, 0177770,
0177700+I_D, 0177400+I_D, 0177700, 0177400,
0177400, 0177000, 0177000, 0177400,
0177400+I_D+I_L, 0170000, 0177777, 0177777 };

GAMEBOY_STATIC const char *opcode[] = {
"HALT","WAIT","RTI","BPT",
"IOT","RESET","RTT","MFPT",
"JMP","RTS","SPL",
"NOP","CLC","CLV","CLV CLC",
"CLZ","CLZ CLC","CLZ CLV","CLZ CLV CLC",
"CLN","CLN CLC","CLN CLV","CLN CLV CLC",
"CLN CLZ","CLN CLZ CLC","CLN CLZ CLC","CCC",
"NOP","SEC","SEV","SEV SEC",
"SEZ","SEZ SEC","SEZ SEV","SEZ SEV SEC",
"SEN","SEN SEC","SEN SEV","SEN SEV SEC",
"SEN SEZ","SEN SEZ SEC","SEN SEZ SEC","SCC",
"SWAB","BR","BNE","BEQ",
"BGE","BLT","BGT","BLE",
"JSR",
"CLR","COM","INC","DEC",
"NEG","ADC","SBC","TST",
"ROL","ROR","ASR","ASL",
"MARK","MFPI","MTPI","SXT",
"CSM",        "TSTSET","WRTLCK",
"MOV","CMP","BIT","BIC",
"BIS","ADD",
"MUL","DIV","ASH","ASHC",
"XOR",
"FADD","FSUB","FMUL","FDIV",
"L2DR",
"MOVC","MOVRC","MOVTC",
"LOCC","SKPC","SCANC","SPANC",
"CMPC","MATC",
"ADDN","SUBN","CMPN","CVTNL",
"CVTPN","CVTNP","ASHN","CVTLN",
"L3DR",
"ADDP","SUBP","CMPP","CVTPL",
"MULP","DIVP","ASHP","CVTLP",
"MOVCI","MOVRCI","MOVTCI",
"LOCCI","SKPCI","SCANCI","SPANCI",
"CMPCI","MATCI",
"ADDNI","SUBNI","CMPNI","CVTNLI",
"CVTPNI","CVTNPI","ASHNI","CVTLNI",
"ADDPI","SUBPI","CMPPI","CVTPLI",
"MULPI","DIVPI","ASHPI","CVTLPI",
"SOB",
"BPL","BMI","BHI","BLOS",
"BVC","BVS","BCC","BCS",
"BHIS","BLO",							/* encode only */
"EMT","TRAP",
"CLRB","COMB","INCB","DECB",
"NEGB","ADCB","SBCB","TSTB",
"ROLB","RORB","ASRB","ASLB",
"MTPS","MFPD","MTPD","MFPS",
"MOVB","CMPB","BITB","BICB",
"BISB","SUB",
"CFCC","SETF","SETI","SETD","SETL",
"LDFPS","STFPS","STST",
"CLRF","CLRD","TSTF","TSTD",
"ABSF","ABSD","NEGF","NEGD",
"MULF","MULD","MODF","MODD",
"ADDF","ADDD","LDF","LDD",
"SUBF","SUBD","CMPF","CMPD",
"STF","STD","DIVF","DIVD",
"STEXP",
"STCFI","STCDI","STCFL","STCDL",
"STCFD","STCDF",
"LDEXP",
"LDCIF","LDCID","LDCLF","LDCLD",
"LDCFD","LDCDF",
NULL };

GAMEBOY_STATIC const int32 opc_val[] = {
0000000+I_NPN, 0000001+I_NPN, 0000002+I_NPN, 0000003+I_NPN,
0000004+I_NPN, 0000005+I_NPN, 0000006+I_NPN, 0000007+I_NPN,
0000100+I_SOP, 0000200+I_REG, 0000230+I_3B,
0000240+I_CCC, 0000241+I_CCC, 0000242+I_CCC, 0000243+I_NPN, 
0000244+I_CCC, 0000245+I_NPN, 0000246+I_NPN, 0000247+I_NPN, 
0000250+I_CCC, 0000251+I_NPN, 0000252+I_NPN, 0000253+I_NPN, 
0000254+I_NPN, 0000255+I_NPN, 0000256+I_NPN, 0000257+I_CCC, 
0000260+I_CCS, 0000261+I_CCS, 0000262+I_CCS, 0000263+I_NPN, 
0000264+I_CCS, 0000265+I_NPN, 0000266+I_NPN, 0000267+I_NPN, 
0000270+I_CCS, 0000271+I_NPN, 0000272+I_NPN, 0000273+I_NPN, 
0000274+I_NPN, 0000275+I_NPN, 0000276+I_NPN, 0000277+I_CCS, 
0000300+I_SOP, 0000400+I_BR, 0001000+I_BR, 0001400+I_BR,
0002000+I_BR, 0002400+I_BR, 0003000+I_BR, 0003400+I_BR,
0004000+I_RSOP,
0005000+I_SOP, 0005100+I_SOP, 0005200+I_SOP, 0005300+I_SOP,
0005400+I_SOP, 0005500+I_SOP, 0005600+I_SOP, 0005700+I_SOP,
0006000+I_SOP, 0006100+I_SOP, 0006200+I_SOP, 0006300+I_SOP,
0006400+I_6B, 0006500+I_SOP, 0006600+I_SOP, 0006700+I_SOP,
0007000+I_SOP,                0007200+I_SOP, 0007300+I_SOP,
0010000+I_DOP, 0020000+I_DOP, 0030000+I_DOP, 0040000+I_DOP,
0050000+I_DOP, 0060000+I_DOP,
0070000+I_RSOP, 0071000+I_RSOP, 0072000+I_RSOP, 0073000+I_RSOP,
0074000+I_RSOP,
0075000+I_REG, 0075010+I_REG, 0075020+I_REG, 0075030+I_REG,
0076020+I_REG,
0076030+I_NPN, 0076031+I_NPN, 0076032+I_NPN,
0076040+I_NPN, 0076041+I_NPN, 0076042+I_NPN, 0076043+I_NPN,
0076044+I_NPN, 0076045+I_NPN, 
0076050+I_NPN, 0076051+I_NPN, 0076052+I_NPN, 0076053+I_NPN,
0076054+I_NPN, 0076055+I_NPN, 0076056+I_NPN, 0076057+I_NPN,
0076060+I_REG,
0076070+I_NPN, 0076071+I_NPN, 0076072+I_NPN, 0076073+I_NPN,
0076074+I_NPN, 0076075+I_NPN, 0076076+I_NPN, 0076077+I_NPN,
0076130+I_NPN, 0076131+I_NPN, 0076132+I_NPN,
0076140+I_NPN, 0076141+I_NPN, 0076142+I_NPN, 0076143+I_NPN,
0076144+I_NPN, 0076145+I_NPN, 
0076150+I_NPN, 0076151+I_NPN, 0076152+I_NPN, 0076153+I_NPN,
0076154+I_NPN, 0076155+I_NPN, 0076156+I_NPN, 0076157+I_NPN,
0076170+I_NPN, 0076171+I_NPN, 0076172+I_NPN, 0076173+I_NPN,
0076174+I_NPN, 0076175+I_NPN, 0076176+I_NPN, 0076177+I_NPN,
0077000+I_SOB,
0100000+I_BR, 0100400+I_BR, 0101000+I_BR, 0101400+I_BR,
0102000+I_BR, 0102400+I_BR, 0103000+I_BR, 0103400+I_BR,
0103000+I_BR, 0103400+I_BR,
0104000+I_8B, 0104400+I_8B,
0105000+I_SOP, 0105100+I_SOP, 0105200+I_SOP, 0105300+I_SOP,
0105400+I_SOP, 0105500+I_SOP, 0105600+I_SOP, 0105700+I_SOP,
0106000+I_SOP, 0106100+I_SOP, 0106200+I_SOP, 0106300+I_SOP,
0106400+I_SOP, 0106500+I_SOP, 0106600+I_SOP, 0106700+I_SOP,
0110000+I_DOP, 0120000+I_DOP, 0130000+I_DOP, 0140000+I_DOP,
0150000+I_DOP, 0160000+I_DOP,
0170000+I_NPN, 0170001+I_NPN, 0170002+I_NPN, 0170011+I_NPN, 0170012+I_NPN,
0170100+I_SOP, 0170200+I_SOP, 0170300+I_SOP,
0170400+I_FOP, 0170400+I_FOP+I_D, 0170500+I_FOP, 0170500+I_FOP+I_D,
0170600+I_FOP, 0170600+I_FOP+I_D, 0170700+I_FOP, 0170700+I_FOP+I_D,
0171000+I_AFOP, 0171000+I_AFOP+I_D, 0171400+I_AFOP, 0171400+I_AFOP+I_D,
0172000+I_AFOP, 0172000+I_AFOP+I_D, 0172400+I_AFOP, 0172400+I_AFOP+I_D, 
0173000+I_AFOP, 0173000+I_AFOP+I_D, 0173400+I_AFOP, 0173400+I_AFOP+I_D,
0174000+I_AFOP, 0174000+I_AFOP+I_D, 0174400+I_AFOP, 0174400+I_AFOP+I_D,
0175000+I_ASOP,
0175400+I_ASMD, 0175400+I_ASMD+I_D, 0175400+I_ASMD+I_L, 0175400+I_ASMD+I_D+I_L, 
0176000+I_AFOP, 0176000+I_AFOP+I_D,
0176400+I_ASOP, 
0177000+I_ASMD, 0177000+I_ASMD+I_D, 0177000+I_ASMD+I_L, 0177000+I_ASMD+I_D+I_L, 
0177400+I_AFOP, 0177400+I_AFOP+I_D,
-1 };

GAMEBOY_STATIC const char *rname [] =
{ "R0", "R1", "R2", "R3", "R4", "R5", "SP", "PC" };

GAMEBOY_STATIC const char *fname [] =
{ "F0", "F1", "F2", "F3", "F4", "F5", "?6", "?7" };

/* Specifier decode

   Inputs:
	addr	=	current PC
	spec	=	specifier
	nval	=	next word
	flag	=	TRUE if decoding for CPU
	iflag	=	TRUE if decoding integer instruction
   Outputs:
	count	=	-number of extra words retired
*/

int32 specf (t_addr addr, int32 spec, t_value nval, int32 flag, int32 iflag)
{
int32 reg;

reg = spec & 07;
switch ((spec >> 3) & 07) {
case 0:
	if (iflag) printf ("%s", rname[reg]);
	else printf ("%s", fname[reg]);
	return 0;
case 1:
	printf ("(%s)", rname[reg]);
	return 0;
case 2:
	if (reg != 7) {
		printf ("(%s)+", rname[reg]);
		return 0;  }
	else {	printf ("#%-o", nval);
		return -1;  }
case 3:
	if (reg != 7) {
		printf ("@(%s)+", rname[reg]);
		return 0;  }
	else {	printf ("@#%-o", nval);
		return -1;  }
case 4:
	printf ("-(%s)", rname[reg]);
	return 0;
case 5:
	printf ("@-(%s)", rname[reg]);
	return 0;
case 6:
	if ((reg != 7) || !flag) printf ("%-o(%s)", nval, rname[reg]);
	else printf ("%-o", (nval + addr + 4) & 0177777);
	return -1;
case 7:
	if ((reg != 7) || !flag) printf ("@%-o(%s)", nval, rname[reg]);
	else printf ("@%-o", (nval + addr + 4) & 0177777);
	return -1;  }					/* end case */
#ifdef GAMEBOY
	return SCPE_ARG;
#endif
}

/* Symbolic decode

   Inputs:
	addr	=	current PC
	*val	=	values to decode
	*uptr	=	pointer to unit
	sw	=	switches
   Outputs:
	return	=	if >= 0, error code
			if < 0, number of extra words retired
*/

t_stat print_sym (t_addr addr, t_value *val, UNIT *uptr, int32 sw)
{
int32 cflag, i, j, c1, c2, inst, fac, srcm, srcr, dstm, dstr;
int32 l8b, brdisp, wd1, wd2;
extern int32 FPS;

cflag = (uptr == NULL) || (uptr == &cpu_unit);
c1 = val[0] & 0177;
c2 = (val[0] >> 8) & 0177;
if (sw & SWMASK ('A')) {				/* ASCII? */
	printf ((c1 < 040)? "<%03o>": "%c", c1);
	return SCPE_OK;  }
if (sw & SWMASK ('C')) {				/* character? */
	printf ((c1 < 040)? "<%03o>": "%c", c1);
	printf ((c2 < 040)? "<%03o>": "%c", c2);
	return SCPE_OK;  }
if (!(sw & SWMASK ('M'))) return SCPE_ARG;

inst = val[0] | ((FPS << (I_V_L - FPS_V_L)) & I_L) |
	((FPS << (I_V_D - FPS_V_D)) & I_D);		/* inst + fp mode */
for (i = 0; opc_val[i] >= 0; i++) {			/* loop thru ops */
    j = (opc_val[i] >> I_V_CL) & I_M_CL;		/* get class */
    if ((opc_val[i] & 0777777) == (inst & masks[j])) {	/* match? */
	srcm = (inst >> 6) & 077;			/* opr fields */
	srcr = srcm & 07;
	fac = srcm & 03;
	dstm = inst & 077;
	dstr = dstm & 07;
	l8b = inst & 0377;

/* Instruction decode */

	switch (j) {					/* case on class */
	case I_V_NPN: case I_V_CCC: case I_V_CCS:	/* no operands */
		printf ("%s", opcode[i]);
		return SCPE_OK;
	case I_V_REG:					/* reg */
		printf ("%s %-s", opcode[i], rname[dstr]);
		return SCPE_OK;
	case I_V_SOP:					/* sop */
		printf ("%s ", opcode[i]);
		return specf (addr, dstm, val[1], cflag, TRUE);
	case I_V_3B:					/* 3b */
		printf ("%s %-o", opcode[i], dstr);
		return SCPE_OK;
	case I_V_FOP:					/* fop */
		printf ("%s ", opcode[i]);
		return specf (addr, dstm, val[1], cflag, FALSE);
	case I_V_AFOP:					/* afop */
		printf ("%s %s,", opcode[i], fname[fac]);
		return specf (addr, dstm, val[1], cflag, FALSE);
	case I_V_6B:					/* 6b */
		printf ("%s %-o", opcode[i], dstm);
		return SCPE_OK;
	case I_V_BR:					/* cond branch */
		printf ("%s ", opcode[i]);
		brdisp = (l8b + l8b + ((l8b & 0200)? 0177002: 2)) & 0177777;
	 	if (cflag) printf ("%-o", (addr + brdisp) & 0177777);
		else if (brdisp < 01000) printf (".+%-o", brdisp);
		else printf (".-%-o", 0200000 - brdisp);
		return SCPE_OK;
	case I_V_8B:					/* 8b */
		printf ("%s %-o", opcode[i], l8b);
		return SCPE_OK;
	case I_V_SOB:					/* sob */
		printf ("%s %s,", opcode[i], rname[srcr]);
		brdisp = (dstm * 2) - 2;
		if (cflag) printf ("%-o", (addr - brdisp) & 0177777);
		else if (brdisp <= 0) printf (".+%-o", -brdisp);
		else printf (".-%-o", brdisp);
		return SCPE_OK;
	case I_V_RSOP:					/* rsop */
		printf ("%s %s,", opcode[i], rname[srcr]);
		return specf (addr, dstm, val[1], cflag, TRUE);
	case I_V_ASOP: case I_V_ASMD:			/* asop, asmd */
		printf ("%s %s,", opcode[i], fname[fac]);
		return specf (addr, dstm, val[1], cflag, TRUE);
	case I_V_DOP:					/* dop */
		printf ("%s ", opcode[i]);
		wd1 = specf (addr, srcm, val[1], cflag, TRUE);
		printf (",");
		wd2 = specf (addr - wd1 - wd1, dstm, val[1 - wd1], cflag, TRUE);
		return wd1 + wd2;  }			/* end case */
		}					/* end if */
	}						/* end for */
return SCPE_ARG;					/* no match */
}

#define A_PND	100					/* # seen */
#define A_MIN	040					/* -( seen */
#define A_PAR	020					/* (Rn) seen */
#define A_REG	010					/* Rn seen */
#define A_PLS	004					/* + seen */
#define A_NUM	002					/* number seen */
#define A_REL	001					/* relative addr seen */

/* Register number

   Inputs:
	*cptr	=	pointer to input string
	*strings =	pointer to register names
	mchar	=	character to match after register name
   Outputs:
	rnum	=	0..7 if a legitimate register
			< 0 if error
*/

int32 get_reg (char *cptr, const char *strings[], char mchar)
{
int32 i;

if (*(cptr + 2) != mchar) return -1;
for (i = 0; i < 8; i++) {
	if (strncmp (cptr, strings[i], 2) == 0) return i;  }
return -1;
}

/* Number or memory address

   Inputs:
	*cptr	=	pointer to input string
	*dptr	=	pointer to output displacement
	*pflag	=	pointer to accumulating flags
   Outputs:
	cptr	=	pointer to next character in input string
			NULL if parsing error

   Flags: 0 (no result), A_NUM (number), A_REL (relative)
*/

char *get_addr (char *cptr, int32 *dptr, int32 *pflag)
{
int32 val, minus;
char *tptr;

minus = 0;

if (*cptr == '.') {					/* relative? */
	*pflag = *pflag | A_REL;
	cptr++;  }
if (*cptr == '+') {					/* +? */
	*pflag = *pflag | A_NUM;
	cptr++;  }
if (*cptr == '-') {					/* -? */
	*pflag = *pflag | A_NUM;
	minus = 1;
	cptr++;  }
errno = 0;
val = strtoul (cptr, &tptr, 8);
if (cptr == tptr) {					/* no number? */
	if (*pflag != (A_REL + A_NUM)) return cptr;
	else return NULL;  }
if (errno || (*pflag == A_REL)) return NULL;		/* .n? */
*dptr = (minus? -val: val) & 0177777;
*pflag = *pflag | A_NUM;
return tptr;
}

/* Specifier decode

   Inputs:
	*cptr	=	pointer to input string
	addr	=	current PC
	n1	=	0 if no extra word used
			-1 if extra word used in prior decode
	*sptr	=	pointer to output specifier
	*dptr	=	pointer to output displacement
	cflag	=	true if parsing for the CPU
	iflag	=	true if integer specifier
   Outputs:
	status	=	= -1 extra word decoded
			=  0 ok
			= +1 error
*/

t_stat get_spec (char *cptr, t_addr addr, int32 n1, int32 *sptr, t_value *dptr,
	int32 cflag, int32 iflag)
{
int32 reg, indir, pflag, disp;

indir = 0;						/* no indirect */
pflag = 0;

if (*cptr == '@') {					/* indirect? */
	indir = 010;
	cptr++;  }
if (*cptr == '#') {					/* literal? */
	pflag = pflag | A_PND;
	cptr++;  }
if (strncmp (cptr, "-(", 2) == 0) {			/* autodecrement? */
	pflag = pflag | A_MIN;
	cptr++;  }
else if ((cptr = get_addr (cptr, &disp, &pflag)) == NULL) return 1;
if (*cptr == '(') {					/* register index? */
	pflag = pflag | A_PAR;
	if ((reg = get_reg (cptr + 1, rname, ')')) < 0) return 1;
	cptr = cptr + 4;
	if (*cptr == '+') {				/* autoincrement? */
		pflag = pflag | A_PLS;
		cptr++;  }  }
else if ((reg = get_reg (cptr, iflag? rname: fname, 0)) >= 0) {
	pflag = pflag | A_REG;
	cptr = cptr + 2;  }
if (*cptr != 0) return 1;				/* all done? */

/* Specifier decode, continued */

switch (pflag) {					/* case on syntax */
case A_REG:						/* Rn, @Rn */
	*sptr = indir + reg;
	return 0;
case A_PAR:						/* (Rn), @(Rn) */
	if (indir) {					/* @(Rn) = @0(Rn) */
		*sptr = 070 + reg;
		*dptr = 0;
		return -1;  }
	else *sptr = 010 + reg;
	return 0;
case A_PAR+A_PLS:					/* (Rn)+, @(Rn)+ */
	*sptr = 020 + indir + reg;
	return 0;
case A_MIN+A_PAR:					/* -(Rn), @-(Rn) */
	*sptr = 040 + indir + reg;
	return 0;
case A_NUM+A_PAR:					/* d(Rn), @d(Rn) */
	*sptr = 060 + indir + reg;
	*dptr = disp;
	return -1;
case A_PND+A_REL: case A_PND+A_REL+A_NUM:		/* #.+n, @#.+n */
	if (!cflag) return 1;
	disp = (disp + addr) & 0177777;			/* fall through */
case A_PND+A_NUM:					/* #n, @#n */
	*sptr = 027 + indir;
	*dptr = disp;
	return -1;
case A_REL: case A_REL+A_NUM:				/* .+n, @.+n */
	*sptr = 067 + indir;
	*dptr = (disp - 4 + (2 * n1)) & 0177777;
	return -1;
case A_NUM:						/* n, @n */
	if (cflag) {					/* CPU - use rel */
		*sptr = 067 + indir;
		*dptr = (disp - addr - 4 + (2 * n1)) & 0177777;  }
	else {	if (indir) return 1;			/* other - use abs */
		*sptr = 037;
		*dptr = disp;  }
	return -1;
default:
	return 1;  }					/* end case */
}

/* Symbolic input

   Inputs:
	*cptr	=	pointer to input string
	addr	=	current PC
	*uptr	=	pointer to unit
	*val	=	pointer to output values
	sw	=	switches
   Outputs:
	status	=	> 0   error code
			<= 0  -number of extra words
*/

t_stat parse_sym (char *cptr, t_addr addr, UNIT *uptr, t_value *val, int32 sw)
{
int32 cflag, d, i, j, reg, spec, n1, n2, disp, pflag;
t_stat r;
char gbuf[CBUFSIZE];

cflag = (uptr == NULL) || (uptr == &cpu_unit);
while (isspace (*cptr)) cptr++;				/* absorb spaces */
if ((sw & SWMASK ('A')) || ((*cptr == '\'') && cptr++)) { /* ASCII char? */
	if (cptr[0] == 0) return SCPE_ARG;		/* must have 1 char */
	val[0] = (t_value) cptr[0];
	return SCPE_OK;  }
if ((sw & SWMASK ('C')) || ((*cptr == '"') && cptr++)) { /* ASCII string? */
	if (cptr[0] == 0) return SCPE_ARG;		/* must have 1 char */
	val[0] = ((t_value) cptr[1] << 8) + (t_value) cptr[0];
	return SCPE_OK;  }

cptr = get_glyph (cptr, gbuf, 0);			/* get opcode */
n1 = n2 = pflag = 0;
for (i = 0; (opcode[i] != NULL) && (strcmp (opcode[i], gbuf) != 0) ; i++) ;
if (opcode[i] == NULL) return SCPE_ARG;
val[0] = opc_val[i] & 0177777;				/* get value */
j = (opc_val[i] >> I_V_CL) & I_M_CL;			/* get class */

switch (j) {						/* case on class */
case I_V_NPN:						/* no operand */
	break;
case I_V_REG:						/* register */
	cptr = get_glyph (cptr, gbuf, 0);		/* get glyph */
	if ((reg = get_reg (gbuf, rname, 0)) < 0) return SCPE_ARG;
	val[0] = val[0] | reg;
	break;
case I_V_3B: case I_V_6B: case I_V_8B:			/* xb literal */
	cptr = get_glyph (cptr, gbuf, 0);		/* get literal */
	d = get_uint (gbuf, 8, (1 << j) - 1, &r);
	if (r != SCPE_OK) return SCPE_ARG;
	val[0] = val[0] | d;				/* put in place */
	break;
case I_V_BR:						/* cond br */
	cptr = get_glyph (cptr, gbuf, 0);		/* get address */
	if ((cptr = get_addr (gbuf, &disp, &pflag)) == NULL) return SCPE_ARG;
	if ((pflag & A_REL) == 0) {
		if (cflag) disp = (disp - addr) & 0177777;
		else return SCPE_ARG;  }
#ifdef GAMEBOY
	if ((disp & 1) || ((disp > 0400) && (disp < 0177402))) return SCPE_ARG;
#else
	if ((disp & 1) || (disp > 0400) && (disp < 0177402)) return SCPE_ARG;
#endif
	val[0] = val[0] | (((disp - 2) >> 1) & 0377);
	break;
case I_V_SOB:						/* sob */
	cptr = get_glyph (cptr, gbuf, ',');		/* get glyph */
	if ((reg = get_reg (gbuf, rname, 0)) < 0) return SCPE_ARG;
	val[0] = val[0] | (reg << 6);
	cptr = get_glyph (cptr, gbuf, 0);		/* get address */
	if ((cptr = get_addr (gbuf, &disp, &pflag)) == NULL) return SCPE_ARG;
	if ((pflag & A_REL) == 0) {
		if (cflag) disp = (disp - addr) & 0177777;
		else return SCPE_ARG;  }
	if ((disp & 1) || ((disp > 2) && (disp < 0177604))) return SCPE_ARG;
	val[0] = val[0] | (((2 - disp) >> 1) & 077);
	break;
case I_V_RSOP:						/* reg, sop */
	cptr = get_glyph (cptr, gbuf, ',');		/* get glyph */
	if ((reg = get_reg (gbuf, rname, 0)) < 0) return SCPE_ARG;
	val[0] = val[0] | (reg << 6);			/* fall through */
case I_V_SOP:						/* sop */
	cptr = get_glyph (cptr, gbuf, 0);		/* get glyph */
	if ((n1 = get_spec (gbuf, addr, 0, &spec, &val[1], cflag, TRUE)) > 0)
		return SCPE_ARG;
	val[0] = val[0] | spec;
	break;
case I_V_AFOP: case I_V_ASOP: case I_V_ASMD:		/* fac, (s)fop */
	cptr = get_glyph (cptr, gbuf, ',');		/* get glyph */
	if ((reg = get_reg (gbuf, fname, 0)) < 0) return SCPE_ARG;
	if (reg > 3) return SCPE_ARG;
	val[0] = val[0] | (reg << 6);			/* fall through */
case I_V_FOP:						/* fop */
	cptr = get_glyph (cptr, gbuf, 0);		/* get glyph */
	if ((n1 = get_spec (gbuf, addr, 0, &spec, &val[1], cflag, 
		(j == I_V_ASOP) || (j == I_V_ASMD))) > 0) return SCPE_ARG;
	val[0] = val[0] | spec;
	break;
case I_V_DOP:						/* double op */
	cptr = get_glyph (cptr, gbuf, ',');		/* get glyph */
	if ((n1 = get_spec (gbuf, addr, 0, &spec, &val[1], cflag, TRUE)) > 0)
		return SCPE_ARG;
	val[0] = val[0] | (spec << 6);
	cptr = get_glyph (cptr, gbuf, 0);		/* get glyph */
	if ((n2 = get_spec (gbuf, addr, n1, &spec, &val[1 - n1],
		cflag, TRUE)) > 0) return SCPE_ARG;
	val[0] = val[0] | spec;
	break;
case I_V_CCC: case I_V_CCS:				/* cond code oper */
	for (cptr = get_glyph (cptr, gbuf, 0); gbuf[0] != 0;
		cptr = get_glyph (cptr, gbuf, 0)) {
		for (i = 0; (opcode[i] != NULL) &&
			(strcmp (opcode[i], gbuf) != 0) ; i++) ;
		if ((((opc_val[i] >> I_V_CL) & I_M_CL) != j) ||
			(opcode[i] == NULL)) return SCPE_ARG;
		val[0] = val[0] | (opc_val[i] & 0177777);  }
	break;
default:
	return SCPE_ARG;  }
if (*cptr != 0) return SCPE_ARG;			/* junk at end? */
return n1 + n2;
}
