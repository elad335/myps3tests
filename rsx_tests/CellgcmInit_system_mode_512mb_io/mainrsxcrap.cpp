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

	if (cellSysmoduleIsLoaded(CELL_SYSMODULE_GCM_SYS) == CELL_SYSMODULE_ERROR_UNLOADED) 
	{
	   cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );
	}

	register int ret asm ("3");

	sys_mmapper_allocate_memory(0x1000000, SYS_MEMORY_GRANULARITY_1M, &mem_id);

	printf("ret is 0x%x, mem_id=0x%x\n", ret, mem_id);

	sys_mmapper_allocate_address(0x10000000, 0x40f, 0x10000000, &addr1);

	printf("ret is 0x%x, addr1=0x%x\n", ret, addr1);

	sys_mmapper_map_memory(u64(addr1), mem_id, SYS_MEMORY_PROT_READ_WRITE);

	printf("ret is 0x%x\n", ret);

	//cellGcmInitSystemMode( CELL_GCM_SYSTEM_MODE_IOMAP_512MB ); 
	ret = cellGcmInit(1 << 18, 0 << 20, ptr_cast(addr1));

	printf("ret is 0x%x\n", ret);

	printf("sample finished.\n");

	return 0;
}