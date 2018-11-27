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

enum {nullptr = 0};

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
	{
		u64 addr;
		system_call_3(SYS_RSX_DEVICE_MAP, int_cast(&addr), nullptr, 8);

		printf("ret is 0x%x, addr = 0x%x\n", ret, addr);
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
		system_call_6(SYS_RSX_CONTEXT_ALLOCATE, int_cast(&context_id), int_cast(&lpar_dma_control), int_cast(&lpar_driver_info), int_cast(&lpar_reports), mem_handle /*0x5a5a5a5b*/, 0x820);

		printf("ret is 0x%x, context_id=0x%x, lpar_dma_control=0x%x, lpar_driver_info=0x%x, lpar_reports=0x%x\n", ret, context_id, lpar_dma_control, lpar_driver_info, lpar_reports);
	}

	sys_mmapper_allocate_memory(0x800000, SYS_MEMORY_GRANULARITY_1M, &mem_id);

	printf("ret is 0x%x, mem_id=0x%x\n", ret, mem_id);

	sys_mmapper_allocate_address(0x10000000, 0x40f, 0x10000000, &addr1);

	printf("ret is 0x%x, addr1=0x%x\n", ret, addr1);

	sys_mmapper_map_memory(u64(addr1), mem_id, SYS_MEMORY_PROT_READ_WRITE);

	printf("ret is 0x%x\n", ret);

	{
		system_call_5(SYS_RSX_CONTEXT_IOMAP, context_id, 0x000000, u64(addr1), 0x00200000 /*size = 2mb*/, 0xe000000000000800ull);
		printf("sys_rsx_context_iomap1: ret is 0x%x\n", ret);
	}

	{
		system_call_6(SYS_RSX_CONTEXT_ATTRIBUTE, context_id, 1 /* FIFO pointers, rsx entry point */, 0, 0, 0, 0);
		printf("sys_rsx_context_attribute: ret is 0x%x\n", ret);
	}

	{
		system_call_6(SYS_RSX_CONTEXT_ATTRIBUTE, context_id, 1 /* FIFO pointers, rsx entry point */, 0x300000, 0, 0, 0);
		printf("sys_rsx_context_attribute: ret is 0x%x\n", ret);
	}

	{
		system_call_6(SYS_RSX_CONTEXT_ATTRIBUTE, context_id, 1 /* FIFO pointers, rsx entry point */, 1u << 31, 0, 0, 0);
		printf("sys_rsx_context_attribute: ret is 0x%x\n", ret);
	}


	//sys_rsx_context_attribute(context_id, 0x300, 0,0,0,0);

	//printf("ret is 0x%x", ret);
	
    printf("sample finished.\n");

    return 0;
}