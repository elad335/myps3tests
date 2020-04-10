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
// Stack	: 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;
sys_addr_t addr;

static rsxCommandCompiler c;

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );

	ENSURE_OK(sys_memory_allocate(0x800000, SYS_MEMORY_GRANULARITY_1M, &addr));

	ENSURE_OK(cellGcmInit(1<<16, 0x100000, ptr_cast(addr)));
	GcmMapEaIoAddress(addr + (1<<20), 0x100000, 0x100000);

	CellGcmControl* ctrl = cellGcmGetControlRegister();

	// Wait for RSX to complete previous operation
	wait_for_fifo(ctrl);

	c.c.begin = 0;
	c.c.current = 0;

	// Place a jump into io address 0
	*OffsetToAddr(ctrl->get) = RSX_METHOD_NEW_JUMP_CMD;
	sys_timer_usleep(40);

	c.call(4); // call-to-next
	c.call(8); // Call-to-next
	c.jmp(12); // Branch-to-self
	fsync();

	ctrl->put = 12;
	sys_timer_usleep(1000);

	// Wait for complition
	while(ctrl->get != 12) sys_timer_usleep(100);

	printf("sample finished.");

	return 0;
}