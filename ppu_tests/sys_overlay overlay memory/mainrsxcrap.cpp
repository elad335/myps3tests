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

#include "../ppu_header.h"

#define MGS_OVERLAY "/app_home/init.self"
enum {nullptr = 0};

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x40000)

sys_memory_t mem_id;
sys_addr_t addr1;
sys_addr_t addr2;
sys_event_queue_t queue_id;

int err = 0;

int main() {


	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_SAVEDATA);
	cellSysmoduleLoadModule(CELL_SYSMODULE_FS);

	sys_addr_t alloc_addr;
	ERROR_CHECK_RET(sys_mmapper_allocate_address(0x10000000, 0x400, 0, &alloc_addr));
	sys_memory_t mem_id, dummy;
	ERROR_CHECK_RET(sys_mmapper_allocate_memory(0x1200000, 0x400, &mem_id));
	ERROR_CHECK_RET(sys_mmapper_map_memory(alloc_addr, mem_id, 0x40000));

	u32 oid, entry;
	u32 err_6[6] = {0};

	// Try to load overlay module in verious memory conditions
	if ((err_6[0] = sys_overlay_load_module(&oid, MGS_OVERLAY, 0, &entry)))
	{
		ERROR_CHECK_RET(sys_mmapper_unmap_memory(alloc_addr, &dummy));

		if ((err_6[1] = sys_overlay_load_module(&oid, MGS_OVERLAY, 0, &entry)))
		{
			ERROR_CHECK_RET(sys_mmapper_free_memory(mem_id));
			ERROR_CHECK_RET(sys_mmapper_free_address(alloc_addr));

			if ((err_6[2] = sys_overlay_load_module(&oid, MGS_OVERLAY, 0, &entry)))
			{
				//printf("Failed to load OVERLAY!, error codes 1=0x%x, 2=0x%x, 3=0x%x\n", err_6[0], err_6[1], err_6[2]);
			}
		}
	}

	sys_mmapper_unmap_memory(alloc_addr, &dummy);
	sys_mmapper_free_memory(mem_id);
	sys_mmapper_free_address(alloc_addr);

	#define ENSURE_ALLOCATED(x) ref_cast((x), volatile u8) += 1;
	// Assume: atleast 128mb + 2 pages of memory is available
	u32 _addr[3] = {0};
	do
	{
		// Allocate half of the block
		err = err_6[3] = sys_memory_allocate(0x9000000, 0x200, &_addr[0]);
		if (err) break;
		ENSURE_ALLOCATED(_addr[0] + 0x9000000 - 1);
		// Allocate another one page
		err = err_6[4] = sys_memory_allocate(0x100000, 0x200, &_addr[1]);
		if (err) break;
		ENSURE_ALLOCATED(_addr[1] + 0x100000 - 1);
		// Free first half
		err = sys_memory_free(_addr[0]);
		if (err) break;
		// Allocate another half plus a little (fails!)
		err = err_6[5] = sys_memory_allocate(0xA000000 + 0x100000, 0x200, &_addr[2]);
		if (err) break;
		ENSURE_ALLOCATED(_addr[2] + 0xA000000 + 0x100000 - 1);
		err = sys_memory_free(_addr[1]);
		if (err) break;
		err = sys_memory_free(_addr[2]);
		if (err) break;
		break;
		printf("Success: memory block check passsed.\n");
	}
	while (0);

	printf("addr1=0x%x, addr2=0x%x, addr3=0x%x"
	", err1=0x%x, err2,=0x%x, err3=0x%x.\n", _addr[0], _addr[1], _addr[2]
	, err_6[3], err_6[4], err_6[5]);
	sys_memory_free(_addr[0]);
	sys_memory_free(_addr[1]);
	sys_memory_free(_addr[2]);

	if (!err_6[0])
	{
		printf("OVERLAY LOAD on memory: success!\n");
	}
	else if (!err_6[1])
	{
		printf("OVERLAY LOAD on memory: success!\n");
	}
	else if (!err_6[2])
	{
		printf("OVERLAY LOAD on empty: success!\n");
	}
	else
	{
		printf("Failed to load OVERLAY!\n");
	}

	printf("OVERLAY error codes: 1=0x%x, 2=0x%x, 3=0x%x\n", err_6[0], err_6[1], err_6[2]);
    printf("sample finished.\n");
    return 0;
}