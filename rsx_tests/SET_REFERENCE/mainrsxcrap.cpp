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

// Table for translation, as we dont have the gcm's
struct RsxIoAddrTable
{
	u16 ea[0x200];
} RSXMem;

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;
sys_addr_t addr1;
sys_addr_t addr2;
sys_event_queue_t queue_id;

int main() {

	register int ret asm ("3");
	u64 addr;
	{
		system_call_3(SYS_RSX_DEVICE_MAP, int_cast(&addr), 0, 8);
	}

	u32 mem_handle;
	u64 mem_addr; 
	{
		system_call_7(SYS_RSX_MEMORY_ALLOCATE, int_cast(&mem_handle), int_cast(&mem_addr), 0xf900000, 0x80000, 0x300000, 0xf, 0x8); //size = 0xf900000, 249 mb

		printf("ret is 0x%x, mem_handle=0x%x, mem_addr=0x%x\n", ret, mem_handle, mem_addr);
	}

	u32 context_id;
	u64 lpar_dma_control, lpar_driver_info, lpar_reports;
	{
		system_call_6(SYS_RSX_CONTEXT_ALLOCATE, int_cast(&context_id), int_cast(&lpar_dma_control), int_cast(&lpar_driver_info), int_cast(&lpar_reports), mem_handle, 0x820);

		printf("ret is 0x%x, context_id=0x%x, lpar_dma_control=0x%x, lpar_driver_info=0x%x, lpar_reports=0x%x\n", ret, context_id, lpar_dma_control, lpar_driver_info, lpar_reports);
	}

	
	
    printf("REF=0x%x, sample finished.\n", ptr_caste(lpar_dma_control, RsxDmaControl)->ref);

    return 0;
}