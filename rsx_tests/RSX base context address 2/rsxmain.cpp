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

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define int_cast(addr) reinterpret_cast<uintptr_t>(addr)
#define ptr_cast(intnum) static_cast<void*>(intnum)

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

// use static to avoid allocations, even though its usually goes into the stack
static sys_memory_t mem_id;
static sys_addr_t addr1;
static u64 ctx_addr;
static u32 mem_handle;
static u64 mem_addr; 

int main() {

	sys_mmapper_allocate_address(0x10000000, 0x20f, 0x10000000, &addr1);
	asm volatile ("twi 0x10, 3, 0"); // Test return value

	{
		system_call_3(SYS_RSX_DEVICE_MAP, int_cast(&ctx_addr), 0, 8);

		asm volatile ("twi 0x10, 3, 0");
	}

	{
		system_call_7(SYS_RSX_MEMORY_ALLOCATE, int_cast(&mem_handle), int_cast(&mem_addr), 0xf900000, 0x80000, 0x300000, 0xf, 0x8); //size = 0xf900000, 249 mb

		asm volatile ("twi 0x10, 3, 0");
	}

	static u32 context_id;
	static u64 lpar_dma_control, lpar_driver_info, lpar_reports;
	{
		system_call_6(SYS_RSX_CONTEXT_ALLOCATE, int_cast(&context_id), int_cast(&lpar_dma_control), int_cast(&lpar_driver_info), int_cast(&lpar_reports), mem_handle /*0x5a5a5a5b*/, 0x820);

		asm volatile ("twi 0x10, 3, 0");
	}

	printf("addr1=0x%x, ctx_addr=0x%x, lpar_dma_control=0x%llx, lpar_driver_info=0x%llx, lpar_reports=0x%llx\n", 
	addr1, ctx_addr, lpar_dma_control, lpar_driver_info, lpar_reports);
    printf("sample finished.\n");

    return 0;
}