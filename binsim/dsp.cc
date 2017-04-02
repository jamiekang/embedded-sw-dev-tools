#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <setjmp.h>
#include <assert.h>
#include <stdarg.h>
#include <map>

#include "vhpi_user.h"
#include "dsp.h"

#ifdef __cplusplus
extern "C" {
#endif
// coming from vhdl simulator
void vhpi_init_dsp(vhpiCbDataT* pCbData);
void vhpi_get_dsp(vhpiCbDataT *pCbData);
void vhpi_put_dsp(vhpiCbDataT *pCbData);
void vhpi_ack_dsp(vhpiCbDataT *pCbData);
#ifdef __cplusplus
}
#endif

// callback function
void dsp_close(vhpiCbDataT *pCbData);

static void assert_msg(bool test, const char * __format, ...) 
{
	va_list ap;
	va_start (ap, __format);

	if(test == false) {
		vprintf(__format, ap);
		exit(-1);
	}

	va_end (ap);
}

static int cur_id;

static struct amba_port {
	vhpiHandleT inport[NUM_M_INPORT];
	vhpiValueT *invalue[NUM_M_INPORT];
	vhpiHandleT outport[NUM_M_OUTPORT];
	vhpiValueT *outvalue[NUM_M_OUTPORT];
} master_insts[MAX_INSTS]; 

static sigjmp_buf sim_env[MAX_INSTS];
static sigjmp_buf usr_env[MAX_INSTS];
static int output_port_init[MAX_INSTS];
static int input_port_init[MAX_INSTS];

// handler type
typedef void (*usr_sw_t)( );
static int gID = 0;
static std::map<char *, int> modTable;
static std::map<int , char *> imodTable;

// previous control signals
static int prev_cmd[MAX_INSTS]; 
//static int prev_priority[MAX_INSTS];
static int prev_write0[MAX_INSTS];
static int prev_wmask0[MAX_INSTS];
static int prev_addr0[MAX_INSTS];
static void *prev_buf0[MAX_INSTS];
static int prev_write1[MAX_INSTS];
static int prev_wmask1[MAX_INSTS];
static int prev_addr1[MAX_INSTS];
static void *prev_buf1[MAX_INSTS];
static usr_sw_t usr_sw_ptr[MAX_INSTS];
static void *handle[MAX_INSTS];

static int wmask_byte[4] = { 0x7, 0xB, 0xD, 0xE };
//static int wmask_hword[2] = { 0x3, 0xC };
static int wmask_hword[2] = { 0x1, 0x2 };

void vhpi_init_dsp(vhpiCbDataT* pCbData)
{
	vhpiHandleT func = pCbData->obj;
	vhpiHandleT process = vhpi_handle(vhpiCurProcess, func);;
	vhpiHandleT target = vhpi_handle(vhpiUpperRegion, process);;
	vhpiCbDataT* CbData;
	char bin_path[256];
	int i, ID;

	ID = gID++;

	output_port_init[ID] = 0;
	input_port_init[ID] = 0;
	prev_cmd[ID] = CMD_IDLE;

	// map process pointer and ID
	modTable[vhpi_get_str(vhpiNameP, target)] = ID;
	imodTable[ID] = vhpi_get_str(vhpiNameP, target);

	// Obtain software binary path
	sprintf(bin_path,"%s.so", vhpi_get_str(vhpiNameP, target));

	vhpi_release_handle(process);
	vhpi_release_handle(target);

	// Register the call-back function for closing the simulation.
	CbData = (vhpiCbDataT*) malloc(sizeof(vhpiCbDataT));
	CbData->reason = vhpiCbEndOfSimulation;
	CbData->cbf = dsp_close;
	vhpi_register_cb(CbData);

	// Initialze port values
	for(i = 0; i < NUM_M_OUTPORT; i++) {
		master_insts[ID].outvalue[i] = (vhpiValueT*) malloc(sizeof(vhpiValueT));
		master_insts[ID].outvalue[i]->format = vhpiIntVal;
		// Note that 0 is ok for every value: e.g. CMD_IDLE, HTRANS_IDLE
		master_insts[ID].outvalue[i]->value.intg = 0;
	}

	for(i = 0; i < NUM_M_INPORT; i++) {
		master_insts[ID].invalue[i] = (vhpiValueT*) malloc(sizeof(vhpiValueT));
		master_insts[ID].invalue[i]->format = vhpiIntVal;
		// Note that 0 is ok for every value: e.g. CMD_IDLE, HTRANS_IDLE
		master_insts[ID].invalue[i]->value.intg = 0;
	}

	// set return value
	master_insts[ID].outvalue[0]->value.intg = 1;
	vhpi_put_value(func, master_insts[ID].outvalue[0], vhpiForce);

	handle[ID] = dlopen(bin_path, RTLD_LAZY);
	assert_msg(handle[ID] != NULL, "cannot open %s\n", bin_path);
	usr_sw_ptr[ID] = (usr_sw_t) dlsym(handle[ID], "main");
	assert_msg(usr_sw_ptr[ID] != NULL, "cannot open main in %s\n", bin_path);

	// memory allocation
#define STACK_SIZE	0x100000*16	// 16MByte
	char *stack = (char *)malloc(STACK_SIZE);
	assert_msg(stack, "unable to allocate stack memory\n");


	// Load target software
	if(sigsetjmp(sim_env[ID], 1) == 0) {
		// create new context
		sigsetjmp(usr_env[ID], 1);
		usr_env[ID]->__jmpbuf[JB_PC] = (int)usr_sw_ptr[ID];
#if 1
		usr_env[ID]->__jmpbuf[JB_BP] = (int)(stack+STACK_SIZE);
		usr_env[ID]->__jmpbuf[JB_SP] = (int)(stack+STACK_SIZE);
#else
		usr_env[ID]->__jmpbuf[JB_RBP] = (int)(stack+STACK_SIZE);
		usr_env[ID]->__jmpbuf[JB_RSP] = (int)(stack+STACK_SIZE);
#endif

		cur_id = ID;	// before initial function call or siglongjmp, set cur_id
		siglongjmp(usr_env[ID], 1);
	}

	//dlclose(handle);
}

// It is called multiple times per a cycle
void vhpi_get_dsp( vhpiCbDataT *pCbData )
{
	int i;
	vhpiHandleT func = pCbData->obj;
	vhpiHandleT process = vhpi_handle(vhpiCurProcess, func);
	vhpiHandleT target = vhpi_handle(vhpiUpperRegion, process);;
	int ID = modTable[vhpi_get_str(vhpiNameP, target)];

	if(output_port_init[ID]==0) {
		// Initialze port values
		for(i = 0; i < NUM_M_OUTPORT; i++) {
			master_insts[ID].outport[i] = vhpi_handle_by_index(vhpiParamDecls, func, i);
		}
		output_port_init[ID]= 1;
	}

    vhpi_get_value(master_insts[ID].outport[0], master_insts[ID].outvalue[0]);

	cur_id = ID;

	for(i = 0; i < NUM_M_OUTPORT; i++) {
		vhpi_put_value(master_insts[ID].outport[i], master_insts[ID].outvalue[i],vhpiForcePropagate);
	}

#if 0
	vhpi_printf("vhpi_get_dsp: %s cmd = %d addr = 0x%x data = 0x%x cycle = 0x%x \n", 
			vhpi_get_str(vhpiNameP, target), 
			master_insts[ID].outvalue[CMD_M_IDX]->value.intg, 
			master_insts[ID].outvalue[ADDR_M_IDX]->value.intg, 
			master_insts[ID].outvalue[WDATA_M_IDX]->value.intg, dsp_get_clock( ));
#endif

}

// To deliver data
void vhpi_put_dsp( vhpiCbDataT *pCbData )
{
	int i;
	vhpiHandleT func = pCbData->obj;
	vhpiHandleT process = vhpi_handle(vhpiCurProcess, func);
	vhpiHandleT target = vhpi_handle(vhpiUpperRegion, process);;
	int ID = modTable[vhpi_get_str(vhpiNameP, target)];

	if(input_port_init[ID]==0) {
		// Initialze port values
		for(i = 0; i < NUM_M_INPORT; i++) {
			master_insts[ID].inport[i] = vhpi_handle_by_index(vhpiParamDecls, func, i);
		}
		input_port_init[ID]= 1;
	}

	//cur_id = ID;
	//vhpi_printf("vhpi_put_dsp: %s cycle = %d prev_cmd[ID] = %d cur_id = %d\n", 
	//				vhpi_get_str(vhpiNameP, target), dsp_get_clock( ), prev_cmd[ID], ID);

	for(i = 0; i < NUM_M_INPORT; i++) {
    	vhpi_get_value(master_insts[ID].inport[i], master_insts[ID].invalue[i]);
//		vhpi_printf(" ID = %d index = %d data = %d \n", ID, i, master_insts[ID].invalue[i]->value.intg);
	}

	// read operation
	if(prev_cmd[ID] == CMD_TRANS) {
		if(prev_write0[ID] == 0) { // read

			// wmask is not necessary, but to save the type(byte/hword/word), we use prev_wmask
/*			if((prev_wmask[ID]&0x8) == 0) 
				((unsigned char *)prev_buf[ID])[0] = (master_insts[ID].invalue[RDATA_M_IDX]->value.intg>>24)&0xFF;

			if((prev_wmask[ID]&0x4) == 0) 
				((unsigned char *)prev_buf[ID])[1] = (master_insts[ID].invalue[RDATA_M_IDX]->value.intg>>16)&0xFF;
*/
			if(prev_wmask0[ID] == 0) // word
				((unsigned int *)prev_buf0[ID])[0] = (master_insts[ID].invalue[RDATA0_M_IDX]->value.intg);

			if(prev_wmask0[ID] == 1) // high half word
				((unsigned short *)prev_buf0[ID])[0] = ((master_insts[ID].invalue[RDATA0_M_IDX]->value.intg<<8)>>20);

			if(prev_wmask0[ID] == 2) // low half word
				((unsigned short *)prev_buf0[ID])[0] = ((master_insts[ID].invalue[RDATA0_M_IDX]->value.intg<<20)>>20);
/*
			vhpi_printf("read (%d, %s): addr = 0x%x rdata = 0x%x (0x%x)wmask = 0x%x cycle = %x\n", 
							ID, imodTable[ID], prev_addr0[ID], ((unsigned short *)prev_buf0[ID])[0], master_insts[ID].invalue[RDATA0_M_IDX]->value.intg, prev_wmask0[ID], dsp_get_clock( ));
*/
		}

		if ( prev_write1[ID] == 0 ) { // read
			if(prev_wmask1[ID] == 0) 
				((unsigned int *)prev_buf1[ID])[0] = (master_insts[ID].invalue[RDATA1_M_IDX]->value.intg);

			if(prev_wmask1[ID] == 1) // high half word
				((unsigned short *)prev_buf1[ID])[0] = ((master_insts[ID].invalue[RDATA1_M_IDX]->value.intg<<8)>>20);

			if(prev_wmask1[ID] == 2) // low half word
				((unsigned short *)prev_buf1[ID])[0] = ((master_insts[ID].invalue[RDATA1_M_IDX]->value.intg<20)>>20);
		}

	}
}

// to patch next control signal, jump to user application code
void vhpi_ack_dsp( vhpiCbDataT *pCbData )
{
	int i;
	vhpiHandleT func = pCbData->obj;
	vhpiHandleT process = vhpi_handle(vhpiCurProcess, func);
	vhpiHandleT target = vhpi_handle(vhpiUpperRegion, process);;
	int ID = modTable[vhpi_get_str(vhpiNameP, target)];

	//vhpi_printf("before user software : %s\n", dsp_get_name( ));
	if(sigsetjmp(sim_env[ID], 1) == 0) {
		cur_id = ID;
		siglongjmp(usr_env[ID], 1);
	}
	//vhpi_printf("after user software: %s\n", dsp_get_name( ));
}

void dsp_close(vhpiCbDataT *pCbData)
{
	exit(1);
}

// Helper function to access port value

static int busy_pattern[4] = { 0, 0, 0, 0};
static unsigned pattern_index[MAX_INSTS];
static unsigned pattern_value[MAX_INSTS];

static void set_signal( 
	int cmd, 
	int enable0, 
	int write0, 
	int wmask0, 
	int addr0, 
	void *buf0,
	int enable1, 
	int write1, 
	int wmask1, 
	int addr1, 
	void *buf1
) 
{

	master_insts[cur_id].outvalue[CMD_M_IDX]->value.intg = cmd;
//	master_insts[cur_id].outvalue[PRIORITY_M_IDX]->value.intg = priority;
	master_insts[cur_id].outvalue[ENABLE0_M_IDX]->value.intg = enable0;
	master_insts[cur_id].outvalue[WRITE0_M_IDX]->value.intg = write0;
	master_insts[cur_id].outvalue[WMASK0_M_IDX]->value.intg = wmask0;
	master_insts[cur_id].outvalue[ADDR0_M_IDX]->value.intg = addr0;
	master_insts[cur_id].outvalue[ENABLE1_M_IDX]->value.intg = enable1;
	master_insts[cur_id].outvalue[WRITE1_M_IDX]->value.intg = write1;
	master_insts[cur_id].outvalue[WMASK1_M_IDX]->value.intg = wmask1;
	master_insts[cur_id].outvalue[ADDR1_M_IDX]->value.intg = addr1;

	// BUSY signal test
#if 1
	// busy generation
	while(pattern_value[cur_id]>0) {

		master_insts[cur_id].outvalue[CMD_M_IDX]->value.intg = CMD_WAIT;
		master_insts[cur_id].outvalue[ADDR0_M_IDX]->value.intg = 1;

		if(sigsetjmp(usr_env[cur_id], 1) == 0) {
			siglongjmp(sim_env[cur_id], 1);
		}

		pattern_value[cur_id]--;
	}

	pattern_index[cur_id] = (pattern_index[cur_id]+1)&0x3;
	pattern_value[cur_id] = busy_pattern[pattern_index[cur_id]];

	master_insts[cur_id].outvalue[CMD_M_IDX]->value.intg = cmd;
	master_insts[cur_id].outvalue[ADDR0_M_IDX]->value.intg = addr0;
#endif

//	vhpi_printf("set_signal of dsp_master: %s : cmd = %d\n", dsp_get_name( ), cmd);

	// write operation
	if(cmd == CMD_TRANS and write0 == 1) {

		master_insts[cur_id].outvalue[WDATA0_M_IDX]->value.intg = 0;

/*		if((wmask&0x8)==0)
			master_insts[cur_id].outvalue[WDATA_M_IDX]->value.intg |= (((unsigned char *)buf)[0]<<24);
		if((wmask&0x4)==0)
			master_insts[cur_id].outvalue[WDATA_M_IDX]->value.intg |= (((unsigned char *)buf)[1]<<16);
*/
		if(wmask0==0) // word
			master_insts[cur_id].outvalue[WDATA0_M_IDX]->value.intg |= (((unsigned int *)buf0)[0]);
		if(wmask0==1) // high half word
			master_insts[cur_id].outvalue[WDATA0_M_IDX]->value.intg |= (((unsigned short *)buf0)[0]<<12);
		if(wmask0==2) // low half word
			master_insts[cur_id].outvalue[WDATA0_M_IDX]->value.intg |= (((unsigned short *)buf0)[0] & 0x000FFF);

/*		vhpi_printf("write (%d, %s): addr = 0x%x wdata = 0x%x wmask = 0x%x cycle = %x\n", 
				cur_id, imodTable[cur_id], addr, master_insts[cur_id].outvalue[WDATA_M_IDX]->value.intg, wmask, dsp_get_clock( ));
*/
	}
	if(cmd == CMD_TRANS and write1 == 1) {

		master_insts[cur_id].outvalue[WDATA1_M_IDX]->value.intg = 0;

		if(wmask1==0) // word
			master_insts[cur_id].outvalue[WDATA1_M_IDX]->value.intg |= (((unsigned int *)buf1)[0]);
		if(wmask1==1) // high half word
			master_insts[cur_id].outvalue[WDATA1_M_IDX]->value.intg |= (((unsigned short *)buf1)[0]<<12);
		if(wmask1==2) // low half word
			master_insts[cur_id].outvalue[WDATA1_M_IDX]->value.intg |= (((unsigned short *)buf1)[0] & 0x000FFF);

	}

	//vhpi_printf("before simulation: %s\n", dsp_get_name( ));
	// because imb is pipelined in read case, at the first time set control signals or wdata
	// after that, read data
	if(sigsetjmp(usr_env[cur_id], 1) == 0) {
		siglongjmp(sim_env[cur_id], 1);
	}
	else {
		prev_cmd[cur_id] = cmd;
//		prev_priority[cur_id] = priority;
		prev_write0[cur_id] = write0;
		prev_wmask0[cur_id] = wmask0;
		prev_addr0[cur_id] = addr0;
		prev_buf0[cur_id] = buf0;
		prev_write1[cur_id] = write1;
		prev_wmask1[cur_id] = wmask1;
		prev_addr1[cur_id] = addr1;
		prev_buf1[cur_id] = buf1;
	}
	//vhpi_printf("after simulation: %s prev_cmd[cur_id] = %d cycle = %x ID=%d\n", dsp_get_name( ), cmd, dsp_get_clock( ), cur_id);
}

// Entered from user software
void dsp_wh(unsigned addr, short *buf, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			1,										// write0
			wmask_hword[((addr+i*1)&0x1)],			// wmask0
			((addr+i*1)>>1),					// addr0
			(void *)&(buf[i]),							// buf0
			0, 0, 3, 0, 0		// enable1, write1, wmask1, addr1, buf1
		);
	}
}

void dsp_ww(unsigned addr, int *buf, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			1,										// write0
			0,										// wmask0
			((addr+i*2)>>1),					// addr0
			(void *)&buf[i],					// buf0
			0, 0, 3, 0, 0);		// enable1, write1, wmask1, addr1, buf1
	}
}

void dsp_rh(unsigned addr, short *buf, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			0,										// write0
			wmask_hword[((addr+i*1)&0x1)],			// wmask0
			((addr+i*1)>>1),					// addr0
			(void *)&buf[i],						// buf0
			0, 0, 3, 0, 0);		// enable1, write1, wmask1, addr1, buf1

	}
}

void dsp_rw(unsigned addr, int *buf, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			0,										// write0
			0,									// wmask0
			((addr+i*2)>>1),					// addr0
			(void *)&buf[i],						// buf0
			0, 0, 3, 0, 0);		// enable1, write1, wmask1, addr1, buf1

	}
}

void dsp_wh_wh(unsigned addr0, short *buf0, unsigned addr1, short *buf1, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			1,										// write0
			wmask_hword[((addr0+i*1)&0x1)],			// wmask0
			((addr0+i*1)>>1),					// addr0
			(void *)&buf0[i],							// buf0
			1, 1, wmask_hword[((addr1+i)&0x1)], (addr1+i)>>1, (void *)&buf1[i]);		// enable1, write1, wmask1, addr1, buf1
	}
}

void dsp_ww_ww(unsigned addr0, int *buf0, unsigned addr1, int *buf1, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			1,										// write0
			0,										// wmask0
			((addr0+i*2)>>1),					// addr0
			(void *)&buf0[i],					// buf0
			1, 1, 0, (addr1+i*2)>>1, (void *)&buf1[i]);		// enable1, write1, wmask1, addr1, buf1
	}
}

void dsp_wh_ww(unsigned addr0, short *buf0, unsigned addr1, int *buf1, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			1,										// write0
			wmask_hword[((addr0+i*1)&0x1)],			// wmask0
			((addr0+i*1)>>1),					// addr0
			(void *)&buf0[i],							// buf0
			1, 1, 0, (addr1+i*2)>>1, (void *)&buf1[i]);		// enable1, write1, wmask1, addr1, buf1
	}
}

void dsp_rh_rh(unsigned addr0, short *buf0, unsigned addr1, short *buf1, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			0,										// write0
			wmask_hword[((addr0+i*1)&0x1)],			// wmask0
			((addr0+i*1)>>1),					// addr0
			(void *)&buf0[i],						// buf0
			1, 0, wmask_hword[((addr1+i)&0x1)], (addr1+i)>>1, (void *)&buf1[i]);		// enable1, write1, wmask1, addr1, buf1

	}
}

void dsp_rw_rw(unsigned addr0, int *buf0, unsigned addr1, int *buf1, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			0,										// write0
			0,										// wmask0
			((addr0+i*2)>>1),					// addr0
			(void *)&buf0[i],						// buf0
			1, 0, 0, (addr1+i)>>1, (void *)&buf1[i]);		// enable1, write1, wmask1, addr1, buf1
	}
}

void dsp_rh_rw(unsigned addr0, short *buf0, unsigned addr1, int *buf1, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			0,										// write0
			wmask_hword[((addr0+i*1)&0x1)],			// wmask0
			((addr0+i*1)>>1),					// addr0
			(void *)&buf0[i],						// buf0
			1, 0, 0, (addr1+i*2)>>1, (void *)&buf1[i]);		// enable1, write1, wmask1, addr1, buf1

	}
}

void dsp_rh_wh(unsigned addr0, short *buf0, unsigned addr1, short *buf1, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			0,										// write0
			wmask_hword[((addr0+i*1)&0x1)],			// wmask0
			((addr0+i*1)>>1),					// addr0
			(void *)&buf0[i],						// buf0
			1, 1, wmask_hword[((addr1+i)&0x1)], (addr1+i)>>1, (void *)&buf1[i]);		// enable1, write1, wmask1, addr1, buf1

	}
}

void dsp_rw_ww(unsigned addr0, int *buf0, unsigned addr1, int *buf1, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			0,										// write0
			0,										// wmask0
			((addr0+i*2)>>1),					// addr0
			(void *)&buf0[i],						// buf0
			1, 1, 0, (addr1+i)>>1, (void *)&buf1[i]);		// enable1, write1, wmask1, addr1, buf1
	}
}

void dsp_rh_ww(unsigned addr0, short *buf0, unsigned addr1, int *buf1, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			0,										// write0
			wmask_hword[((addr0+i*1)&0x1)],			// wmask0
			((addr0+i*1)>>1),					// addr0
			(void *)&buf0[i],						// buf0
			1, 1, 0, (addr1+i*2)>>1, (void *)&buf1[i]);		// enable1, write1, wmask1, addr1, buf1
	}
}

void dsp_rw_wh(unsigned addr0, int *buf0, unsigned addr1, short *buf1, unsigned size)
{
	int i;

	for(i = 0; i<size; i++) {
		set_signal(CMD_TRANS, 						// cmd
			1,										// enable0
			0,										// write0
			0,									// wmask0
			((addr0+i*2)>>1),					// addr0
			(void *)&buf0[i],						// buf0
			1, 1, wmask_hword[((addr1+i*1)&0x1)], (addr1+i*1)>>1, (void *)&buf1[i]);		// enable1, write1, wmask1, addr1, buf1
	}
}


void dsp_wait(unsigned cycle)
{
	set_signal(CMD_WAIT, 						// cmd
		0,										// enable0
		0,										// write0
		0,										// wmask0
		cycle,									// addr0
		0,										// buf0
			0, 0, 3, 0, 0);		// enable1, write1, wmask1, addr1, buf1
}

unsigned int dsp_get_clock(void) 
{
	return master_insts[cur_id].outvalue[0]->value.intg;
}

char *dsp_get_name(void) 
{
	return imodTable[cur_id];
}
