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
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

// use static to avoid allocations, even though its usually goes into the stack
static sys_memory_t mem_id;
static u64 dev_addr;
static u32 mem_handle;
static u64 mem_addr;
register u32 error_code asm ("3");

u32 sys_rsx_memory_allocate(u32* mem_handle, u64* mem_addr)
{
	system_call_7(SYS_RSX_MEMORY_ALLOCATE, int_cast(mem_handle), int_cast(mem_addr), 0xf900000, 0x80000, 0x300000, 0xf, 0x8);
	return_to_user_prog(u32);
}
u32 sys_rsx_context_allocate(u32* context_id, u64* lpar_dma_control, u64* lpar_driver_info, u64* lpar_reports, u32 mem_handle)
{
	system_call_6(SYS_RSX_CONTEXT_ALLOCATE, int_cast(context_id), int_cast(lpar_dma_control), int_cast(lpar_driver_info), int_cast(lpar_reports), mem_handle /*0x5a5a5a5b*/, 0x820);
	return_to_user_prog(u32);
}

u32 sys_rsx_device_map(u64* dev_addr)
{
	system_call_3(SYS_RSX_DEVICE_MAP, int_cast(dev_addr), 0, 8);
	return_to_user_prog(u32);
}

int main() {

	// Ensure 1mb allocation memory block is allocated so it wont mess with printf
	{u32 dummy; sys_memory_allocate(0x1000000, 0x400, &dummy); asm volatile ("twi 0x10, 3, 0");}

	u32 context_id;
	u64 lpar_dma_control, lpar_driver_info, lpar_reports;
	printf("sys_rsx_context_allocate before sys_rsx_memory_allocate returned: 0x%x\n", sys_rsx_context_allocate(&context_id, &lpar_dma_control, &lpar_driver_info, &lpar_reports, mem_handle));

	if (sys_rsx_memory_allocate(&mem_handle, &mem_addr) != CELL_OK) return 0;
	{u32 dummy; u64 dummy2; printf("sys_rsx_memory_allocate second call error code: 0x%x\n", sys_rsx_memory_allocate(&dummy, &dummy2));}

	sys_rsx_context_allocate(&context_id, &lpar_dma_control, &lpar_driver_info, &lpar_reports, mem_handle);

	sys_rsx_device_map(&dev_addr);
	{u64 dummy; printf("sys_rsx_device_map second call returned: 0x%x\n", sys_rsx_device_map(&dev_addr));}

	printf("dev_addr=0x%x, lpar_dma_control=0x%llx, lpar_driver_info=0x%llx, lpar_reports=0x%llx\n", 
	dev_addr, lpar_dma_control, lpar_driver_info, lpar_reports);
    printf("sample finished.\n");

    return 0;
}