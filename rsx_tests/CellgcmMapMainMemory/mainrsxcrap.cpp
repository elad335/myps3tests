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

sys_addr_t addr1;
sys_addr_t addr2;
sys_event_queue_t queue_id;

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );

	ENSURE_OK(sys_memory_allocate(0x1000000, SYS_MEMORY_GRANULARITY_1M, &addr1));

	ENSURE_OK(cellGcmInit(1 << 18, 1 << 20, ptr_cast(addr1)));

	printf("ret is 0x%x\n", g_ec);

	u32 offset;

	g_ec = cellGcmMapMainMemory(ptr_cast(addr1 + (2u<<20)), 7u << 20, &offset);

	printf("ret is 0x%x, offset=0x%x\n", g_ec, offset);

	g_ec = cellGcmUnmapIoAddress(offset + (1u<<20));

	printf("ret is 0x%x, offset=0x%x\n", g_ec);
	
	printf("sample finished.\n");

	return 0;
}