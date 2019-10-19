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
#include <memory>
#include <stdint.h>
#include <string.h>

#include "../rsx_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;
sys_addr_t addr;

static rsxCommandCompiler c;
static CellGcmContextData& Gcm = c.c;

// CellGcmContextCallback
int GcmCallback(struct CellGcmContextData *, uint32_t)
{

}

enum {
	gcmDisplay = 0xC0200000,
};

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );

	sys_memory_allocate(0x800000, 0x400, &addr);

	ENSURE_OK(cellGcmInit(1<<16, 0x100000, ptr_cast(addr)));
	cellGcmMapEaIoAddress(ptr_cast(addr + (1<<20)), 1<<20, 7<<20);
	u8 id;
	cellGcmGetCurrentDisplayBufferId(&id);
	printf("display buffer id:0x%x\n", id);

 	ENSURE_OK(cellGcmSetDisplayBuffer(id, 2<<20, 1280*4, 1280, 720));

	cellGcmGetOffsetTable(&offsetTable);

	const CellGcmDisplayInfo* disp_info = cellGcmGetDisplayInfo();
	CellGcmControl* ctrl = cellGcmGetControlRegister();
	// Wait for RSX to complete previous operation
	wait_for_fifo(ctrl);

	// Place a jump into io address 1mb
	*OffsetToAddr(ctrl->get) = (1<<20) | RSX_METHOD_NEW_JUMP_CMD;
	sys_timer_usleep(40);

	cellGcmSetupContextData(&Gcm, ptr_caste(addr + (1<<20), u32), 0x10000, GcmCallback);

	// Start the Dump
	// Dumped using RPCS3's log:
	// LOG_WARNING(RSX, "c.push(1, 0x%x); c.push(0x%x)", get_method_name(reg), value);
	// While iterating through methods writes loop
	cellGcmSetReferenceCommand(&Gcm, 1);
	cellGcmSetFlip(&Gcm, id);

	// Stop dumping and ack finish
	cellGcmSetReferenceCommand(&Gcm, 2);
	fsync();

	ctrl->put = c.newLabel().pos;
	sys_timer_usleep(100);

	while (ctrl->ref != 2) sys_timer_usleep(1000);

	sys_timer_sleep(1);

	printf("sample finished.");

	return 0;
}