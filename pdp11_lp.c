/* pdp11_lp.c: PDP-11 line printer simulator

   Copyright (c) 1993-1997,
   Robert M Supnik, Digital Equipment Corporation
   Commercial use prohibited

   lpt		line printer
*/

#include "pdp11_defs.h"

#define LPTCSR_IMP	(CSR_ERR + CSR_DONE + CSR_IE)	/* implemented */
#define LPTCSR_RW	(CSR_IE)			/* read/write */

extern int32 int_req;
int32 lpt_csr = 0;					/* control/status */
int32 lpt_stopioe = 0;					/* stop on error */
t_stat lpt_svc (UNIT *uptr);
t_stat lpt_reset (DEVICE *dptr);
#ifdef GAMEBOY
t_stat lpt_identify();
#endif
t_stat lpt_attach (UNIT *uptr, char *ptr);
t_stat lpt_detach (UNIT *uptr);
extern t_stat sim_activate (UNIT *uptr, int32 delay);
extern t_stat sim_cancel (UNIT *uptr);

/* LPT data structures

   lpt_dev	LPT device descriptor
   lpt_unit	LPT unit descriptor
   lpt_reg	LPT register list
*/

UNIT lpt_unit = {
	UDATA (&lpt_svc, UNIT_SEQ+UNIT_ATTABLE, 0), SERIAL_OUT_WAIT };

REG lpt_reg[] = {
	{ ORDATA (LPCS, lpt_csr, 16) },
	{ ORDATA (LPB, lpt_unit.buf, 8) },
	{ FLDATA (INT, int_req, INT_V_LPT) },
	{ FLDATA (ERR, lpt_csr, CSR_V_ERR) },
	{ FLDATA (DONE, lpt_csr, CSR_V_DONE) },
	{ FLDATA (IE, lpt_csr, CSR_V_IE) },
	{ DRDATA (POS, lpt_unit.pos, 31), PV_LEFT },
	{ DRDATA (TIME, lpt_unit.wait, 24), PV_LEFT },
	{ FLDATA (STOP_IOE, lpt_stopioe, 0) },
	{ NULL }  };

DEVICE lpt_dev = {
	"LPT", &lpt_unit, lpt_reg, NULL,
	1, 10, 31, 1, 8, 8,
	NULL, NULL, &lpt_reset,
#ifdef GAMEBOY
	NULL, &lpt_attach, &lpt_detach, &lpt_identify };
#else
	NULL, &lpt_attach, &lpt_detach };
#endif

/* Line printer routines

   lpt_rd	I/O page read
   lpt_wr	I/O page write
   lpt_svc	process event (printer ready)
   lpt_reset	process reset
   lpt_attach	process attach
   lpt_detach	process detach
*/

t_stat lpt_rd (int32 *data, int32 PA, int32 access)
{
if ((PA & 02) == 0) *data = lpt_csr & LPTCSR_IMP;	/* csr */
else *data = lpt_unit.buf;				/* buffer */
return SCPE_OK;
}

t_stat lpt_wr (int32 data, int32 PA, int32 access)
{
if ((PA & 02) == 0) {					/* csr */
	if (PA & 1) return SCPE_OK;
	if ((data & CSR_IE) == 0) int_req = int_req & ~INT_LPT;
	else if ((lpt_csr & (CSR_DONE + CSR_IE)) == CSR_DONE)
		int_req = int_req | INT_LPT;
	lpt_csr = (lpt_csr & ~LPTCSR_RW) | (data & LPTCSR_RW);  }
else {	if ((PA & 1) == 0) lpt_unit.buf = data & 0177;	/* buffer */
	lpt_csr = lpt_csr & ~CSR_DONE;
	int_req = int_req & ~INT_LPT;
	if ((lpt_unit.buf == 015) || (lpt_unit.buf == 014) ||
	    (lpt_unit.buf == 012)) sim_activate (&lpt_unit, lpt_unit.wait);
	else sim_activate (&lpt_unit, 0);  }
return SCPE_OK;
}

t_stat lpt_svc (UNIT *uptr)
{
lpt_csr = lpt_csr | CSR_ERR | CSR_DONE;
if (lpt_csr & CSR_IE) int_req = int_req | INT_LPT;
if ((lpt_unit.flags & UNIT_ATT) == 0)
	return IORETURN (lpt_stopioe, SCPE_UNATT);
if (putc (lpt_unit.buf & 0177, lpt_unit.fileref) == EOF) {
	perror ("LPT I/O error");
	clearerr (lpt_unit.fileref);
	return SCPE_IOERR;  }
lpt_csr = lpt_csr & ~CSR_ERR;
lpt_unit.pos = ftell (lpt_unit.fileref);
return SCPE_OK;
}

t_stat lpt_reset (DEVICE *dptr)
{
lpt_unit.buf = 0;
lpt_csr = CSR_DONE;
if ((lpt_unit.flags & UNIT_ATT) == 0) lpt_csr = lpt_csr | CSR_ERR;
int_req = int_req & ~INT_LPT;
sim_cancel (&lpt_unit);					/* deactivate unit */
return SCPE_OK;
}

t_stat lpt_attach (UNIT *uptr, char *cptr)
{
t_stat reason;

lpt_csr = lpt_csr & ~CSR_ERR;
reason = attach_unit (uptr, cptr);
if ((lpt_unit.flags & UNIT_ATT) == 0) lpt_csr = lpt_csr | CSR_ERR;
return reason;
}

t_stat lpt_detach (UNIT *uptr)
{
lpt_csr = lpt_csr | CSR_ERR;
return detach_unit (uptr);
}

#ifdef GAMEBOY
t_stat
lpt_identify()
{
    printf("LPT: LP11 line printer\n");
    return SCPE_OK;
}
#endif
