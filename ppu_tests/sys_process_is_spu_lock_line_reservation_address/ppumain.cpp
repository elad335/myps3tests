#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_syscache.h>
#include <sys/spu_initialize.h> 
#include <cell/sysmodule.h>
#include <cell/gcm.h>
#include <cell/atomic.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/raw_spu.h>
#include <sys/syscall.h>
#include <sys/vm.h> 
#include <functional>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

struct two_addr
{
	u32 mapped;
	u32 unmapped;
};

two_addr find_mapped_and_unmapped_address_in_region(u32 region_addr)
{
	two_addr ret = {-1u, -1u};

	region_addr &= 0xf0000000; // Align it
	do
	{
		sys_page_attr_t attr;

		switch (sys_memory_get_page_attribute(region_addr, &attr))
		{
		case EINVAL:
		{
			if (ret.unmapped == -1)
			{
				ret.unmapped = region_addr;
			}

			break;
		}
		case CELL_OK:
		{
			if (ret.mapped == -1)
			{
				ret.mapped = region_addr;
			}

			break;
		}
		default: ENSURE_OK(1);
		}

		region_addr += 4096;

	} while (region_addr % 0x10000000);

	return ret;
}

int main() {

	u32 vm_addr, mmapper_addr, mem_addr, no_addr;
	sys_memory_t mem_id;
	sys_raw_spu_t raw_id;
	ENSURE_OK(sys_vm_memory_map(0x2000000, 0x100000, -1, 0x400, 1, &vm_addr));
	ENSURE_OK(sys_mmapper_allocate_address(0x10000000, SYS_MEMORY_PAGE_SIZE_64K, 0x10000000, &mmapper_addr));
	ENSURE_OK(sys_mmapper_allocate_memory(0x100000, SYS_MEMORY_GRANULARITY_64K, &mem_id));
	ENSURE_OK(sys_mmapper_map_memory(mmapper_addr, mem_id, SYS_MEMORY_PROT_READ_WRITE));
	ENSURE_OK(sys_memory_allocate(1 << 20, 0x400, &mem_addr));
	ENSURE_OK(sys_spu_initialize(1, 1));
	ENSURE_OK(sys_raw_spu_create(&raw_id, NULL)); // Allocate raw spu region
	ENSURE_OK(cellGcmInit(1<<18, 1<<20, (void*)mem_addr));
	ENSURE_OK(sys_mmapper_allocate_address(0x10000000, SYS_MEMORY_PAGE_SIZE_64K, 0x10000000, &no_addr));
	ENSURE_OK(sys_mmapper_free_address(no_addr)); // Ensure it does not exist

	for (u32 i = 0 ; i < 16; i++)
	{
		two_addr ret = find_mapped_and_unmapped_address_in_region(i << 28);

		if (ret.mapped != -1)
		{
			printf("Testing mapped address 0x%x\n", ret.mapped);
			justFunc(sys_process_is_spu_lock_line_reservation_address, ret.mapped, SYS_MEMORY_ACCESS_RIGHT_SPU_THR);
		}

		if (ret.unmapped != -1)
		{
			printf("Testing unmapped address: 0x%x\n", ret.unmapped);
			justFunc(sys_process_is_spu_lock_line_reservation_address, ret.unmapped, SYS_MEMORY_ACCESS_RIGHT_SPU_THR);
		}
	}
	return 0;
}