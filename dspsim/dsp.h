//-----------------------------------------------------------------------------
//-- Description	: VHPI IMB Master
//-- Date			: 2007. 3. 7.
//-- Software APIs provided by IMB Master wrapper
//-----------------------------------------------------------------------------

#ifndef __DSP__
#define __DSP__

#ifdef __cplusplus
extern "C" {
#endif
void dsp_wh(unsigned addr, short *buffer, unsigned size);
void dsp_ww(unsigned addr, int *buffer, unsigned size);
void dsp_rh(unsigned addr, short *buffer, unsigned size);
void dsp_rw(unsigned addr, int *buffer, unsigned size);

void dsp_wh_wh(unsigned addr, short *buffer, unsigned addr1, short *buf1, unsigned size);
void dsp_ww_ww(unsigned addr, int *buffer, unsigned addr1, int *buf1, unsigned size);
void dsp_wh_ww(unsigned addr, short *buffer, unsigned addr1, int *buf1, unsigned size);
void dsp_rh_rh(unsigned addr, short *buffer, unsigned addr1, short *buf1, unsigned size);
void dsp_rw_rw(unsigned addr, int *buffer, unsigned addr1, int *buf1, unsigned size);
void dsp_rh_rw(unsigned addr, short *buffer, unsigned addr1, int *buf1, unsigned size);
void dsp_rh_wh(unsigned addr, short *buffer, unsigned addr1, short *buf1, unsigned size);
void dsp_rw_ww(unsigned addr, int *buffer, unsigned addr1, int *buf1, unsigned size);
void dsp_rh_ww(unsigned addr, short *buffer, unsigned addr1, int *buf1, unsigned size);
void dsp_rw_wh(unsigned addr, int *buffer, unsigned addr1, short *buf1, unsigned size);

void dsp_wait(unsigned cycle);
unsigned int dsp_get_clock(void);
char *dsp_get_name(void);
#ifdef __cplusplus
}
#endif

#define NUM_M_OUTPORT	12
#define NUM_M_INPORT	2
#define MAX_INSTS		256

// Output port number
#define SD_M_IDX		0
#define CMD_M_IDX		1
#define ENABLE0_M_IDX	2
#define WRITE0_M_IDX	3
#define WMASK0_M_IDX	4
#define ADDR0_M_IDX		5
#define WDATA0_M_IDX	6
#define ENABLE1_M_IDX	7
#define WRITE1_M_IDX	8
#define WMASK1_M_IDX	9
#define ADDR1_M_IDX		10
#define WDATA1_M_IDX	11

// Input port number
#define RDATA0_M_IDX	0
#define RDATA1_M_IDX	1

// VHPI CMD related constants
#define CMD_IDLE		0
#define CMD_TRANS		1
#define CMD_WAIT		2

/************************************************************************
************************************************************************/

#define	DEFAULTASM		"dsp.asm"
#define	DEFAULTBIN		"dsp.out"

#ifndef JB_BX
# define JB_BX  0
#endif

#ifndef JB_SI
# define JB_SI  1
#endif

#ifndef JB_DI
# define JB_DI  2
#endif

#ifndef JB_BP
# define JB_BP  3
#endif

#ifndef JB_SP
# define JB_SP  4
#endif

#ifndef JB_PC
# define JB_PC  5
#endif

#ifndef JB_SIZE
# define JB_SIZE 24
#endif



#endif
