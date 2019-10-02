#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_sysparam.h> 
#include <sysutil/sysutil_syscache.h>
#include <cell/sysmodule.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <cell/gcm.h>
#include <memory>
#include <stdint.h>
#include <string.h>

#include "../rsx_header.h"

#define trap_syscall() asm volatile ("twi 0x10, 3, 0");

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;
sys_addr_t addr;

static union
{
	CellGcmContextData Gcm;
	rsxCommandCompiler c;
};

// CellGcmContextCallback
int GcmCallback(struct CellGcmContextData *, uint32_t)
{

}

enum {
	gcmDisplay = 0xC0200000,
};

int main() {

	// They should be at the same address (Traps are not gay)
	if (int_cast(&Gcm) != int_cast(&c.c)) asm volatile ("tw 4, 1, 1");

	register int ret asm ("3");

	if (cellSysmoduleIsLoaded(CELL_SYSMODULE_GCM_SYS) == CELL_SYSMODULE_ERROR_UNLOADED) 
	{
	   cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );
	}

	sys_memory_allocate(0x800000, 0x400, &addr);
	trap_syscall();

	cellGcmInit(1<<16, 0x100000, ptr_cast(addr));
	cellGcmMapEaIoAddress(ptr_cast(addr + (1<<20)), 1<<20, 7<<20);
	u8 id;
	cellGcmGetCurrentDisplayBufferId(&id);

 	cellGcmSetDisplayBuffer(id, 2<<20, 1280*4, 1280, 720);
	trap_syscall();

	cellGcmGetOffsetTable(&offsetTable);

	CellGcmDisplayInfo* info = const_cast<CellGcmDisplayInfo*>(cellGcmGetDisplayInfo());
	CellGcmControl* ctrl = cellGcmGetControlRegister();
	// Wait for RSX to complete previous operation
	do sys_timer_usleep(200); while (ctrl->get != ctrl->put);

	// Place a jump into io address 1mb
	*OffsetToAddr(ctrl->get) = (1<<20) | RSX_METHOD_NEW_JUMP_CMD;
	sys_timer_usleep(40);

	cellGcmSetupContextData(&Gcm, ptr_caste(addr + (1<<20), u32), 0x10000, GcmCallback);

	cellGcmSetCullFace(&Gcm, CELL_GCM_FRONT);

	// Note: c and Gcm are basically the same object
	c.push(GCM_FLIP_HEAD);
	c.push(0);

	c.push(GCM_FLIP_HEAD + 4); // Flip HEAD 1 as well
	c.push(0);

	cellGcmSetReferenceCommand(&Gcm, 1);

	fsync();

	ctrl->put = c.newLabel().pos;
	sys_timer_usleep(100);

	while (ctrl->ref != 1) sys_timer_usleep(1000);

	// Crash intentionally
	cellGcmUnmapIoAddress(0);
	cellGcmUnmapIoAddress(1<<20);
	ctrl->put = 0;

	while(true)
	{
		sys_timer_sleep(1);
	}

	return 0;
}