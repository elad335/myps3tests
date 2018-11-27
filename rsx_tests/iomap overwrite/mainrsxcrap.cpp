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
#include <memory>

#include "../rsx_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

static sys_memory_t mem_id;
static sys_addr_t addr;

int main() {

	register int ret asm ("3");

	if (cellSysmoduleIsLoaded(CELL_SYSMODULE_GCM_SYS) == CELL_SYSMODULE_ERROR_UNLOADED) 
	{
	   cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );
	}

	sys_mmapper_allocate_memory(0x800000, SYS_MEMORY_GRANULARITY_1M, &mem_id);

	asm volatile ("twi 0x10, 3, 0");

	sys_mmapper_allocate_address(0x10000000, 0x40f, 0x10000000, &addr);

	asm volatile ("twi 0x10, 3, 0");

	sys_mmapper_map_memory(u64(addr), mem_id, SYS_MEMORY_PROT_READ_WRITE);

	asm volatile ("twi 0x10, 3, 0");

	cellGcmInit(1<<16, 0x100000, ptr_cast(addr));

	// Map io twice, into different addresses
	cellGcmMapEaIoAddress(ptr_cast(addr + (1<<20)), 0x100000, 0x100000);
	cellGcmMapEaIoAddress(ptr_cast(addr + (2<<20)), 0x100000, 0x100000);

	CellGcmControl* ctrl = cellGcmGetControlRegister();

	// Wait for RSX to complete previous operation
	while (ctrl->get != ctrl->put) sys_timer_usleep(400);

	// Place a jump into io address 1mb
	ptr_caste(addr, u32)[ctrl->get / 4] = (1<<20) | RSX_METHOD_NEW_JUMP_CMD;

	u32* cmd = ptr_caste(addr + (1<<20), u32);

	// Test which command is executed
	// Ref = 1: the io offset's EA hasn't changed by the second map
	// Ref = 2: the io offset's EA was updated by the second map

	// NV406E_SET_REFERENCE
	cmd[0] = 0x40050;
	cmd[0x100000 / 4] = 0x40050;

	// Value for ref cmd
	cmd[1] = 1;
	cmd[(0x100000 / 4) + 1] = 2;

	// Branch to self
	cmd[2] = (1<<20) | RSX_METHOD_NEW_JUMP_CMD | 0x8;
	cmd[(0x100000 / 4) + 2] = (1<<20) | RSX_METHOD_NEW_JUMP_CMD | 0x8;

	asm volatile ("eieio;sync");

	ctrl->put = 0x10000C; // Queue: 0x100000...0x10000C

	while(ctrl->ref == -1u) sys_timer_usleep(400);

	printf("Ref=0x%x, Get=0x%x\n", ctrl->ref, ctrl->get);

	// test how the RSX manages io translation internally
	// Crash: unmapping and mapping overwrites previous maps, multiple maps are not possible 
	// No crash: the RSX remembered the two maps made, and when unmapping one, it didnt crash because of the second
	cellGcmUnmapIoAddress(1<<20);
	while(1) sys_timer_sleep(10);

    return 0;
}