#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_syscache.h>
#include <cell/sysmodule.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <cell/gcm.h>
#include <memory>

#include "../rsx_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack	: 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;
sys_addr_t addr;

static rsxCommandCompiler c;
static CellGcmContextData& Gcm = c.c;

// CellGcmContextCallback
int GcmCallback(struct CellGcmContextData *, uint32_t){}

int main() {

	register int ret asm ("3");

	cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );

	sys_mmapper_allocate_memory(0x800000, SYS_MEMORY_GRANULARITY_1M, &mem_id);

	printf("ret is 0x%x, mem_id=0x%x\n", ret, mem_id);

	sys_mmapper_allocate_address(0x10000000, 0x40f, 0x10000000, &addr);

	printf("ret is 0x%x, addr=0x%x\n", ret, addr);

	sys_mmapper_map_memory(u64(addr), mem_id, SYS_MEMORY_PROT_READ_WRITE);

	printf("ret is 0x%x\n", ret);

	ENSURE_OK(cellGcmInit(1<<16, 0x100000, ptr_cast(addr)));

	CellGcmControl* ctrl = cellGcmGetControlRegister();

	// Wait for RSX to complete previous operation
	wait_for_fifo(ctrl);

	// Place a jump into io address 0
	cellGcmSetupContextData(&Gcm, ptr_caste(addr + (0), u32), 0x10000, GcmCallback);
	sys_timer_usleep(40);

	c.push(0x40053); // unaligned NV406E_SET_REFERENCE cmd
	c.push(0x1234); // Value for ref cmd

	ctrl->put = 0x8;
	sys_timer_usleep(1000);

	// Wait for complition
	while(ctrl->get != 8) sys_timer_usleep(100);

	printf("sample finished. REF=0x%x", ctrl->ref);

	return 0;
}