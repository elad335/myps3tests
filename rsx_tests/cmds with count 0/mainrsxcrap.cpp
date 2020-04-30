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
#include <string.h>

#include "../rsx_header.h"

#define SYS_APP_HOME "/app_home"

#define VP_PROGRAM SYS_APP_HOME "/mainvp.vpo"
#define FP_PROGRAM SYS_APP_HOME "/mainfp.fpo"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

static sys_memory_t mem_id;
static sys_addr_t addr;

static inline void LoadModules()
{
	int ret = cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );
	ret |= cellSysmoduleLoadModule( CELL_SYSMODULE_FS );
	ret |= cellSysmoduleLoadModule( CELL_SYSMODULE_RESC );
	ENSURE_OK(ret != CELL_OK && ret != CELL_SYSMODULE_ERROR_DUPLICATED);
}

static rsxCommandCompiler c;
static CellGcmContextData& Gcm = c.c;

// CellGcmContextCallback
int GcmCallback(struct CellGcmContextData *, uint32_t){}

int main() {

	LoadModules();
	sys_memory_allocate(0x1000000, 0x400, &addr);

	ENSURE_OK(cellGcmInit(1<<16, 0x100000, ptr_cast(addr)));
	GcmMapEaIoAddress(addr + (1<<20), 1<<20, 7<<20);

	u8 id; cellGcmGetCurrentDisplayBufferId(&id);
 	cellGcmSetDisplayBuffer(id, 2<<20, 1280*4, 1280, 720);
	cellGcmGetOffsetTable(&offsetTable);

	const CellGcmDisplayInfo* disp_info = cellGcmGetDisplayInfo();
	CellGcmControl* ctrl = cellGcmGetControlRegister();

	// Wait for RSX to complete previous operation
	wait_for_fifo(ctrl);

	// Place a jump into io address 1mb
	*OffsetToAddr(ctrl->get) = (1<<20) | RSX_METHOD_NEW_JUMP_CMD;
	sys_timer_usleep(40);

	cellGcmSetupContextData(&Gcm, ptr_caste(addr + (1<<20), u32), 0x10000, GcmCallback);

	// Usual command
	c.push(1, NV406E_SET_REFERENCE);
	c.push(0x1234);

	// Push set reference with zero count
	// If the command aint a nop, REF will be zero
	c.push(0, NV406E_SET_REFERENCE);
	c.push(0);

	c.flush();
	sys_timer_sleep(2);

	printf("Ref=0x%x.\n sample finished.\n", ctrl->ref);

	return 0;
}