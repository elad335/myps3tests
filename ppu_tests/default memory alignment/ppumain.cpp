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

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define int_cast(addr) reinterpret_cast<uptr>(addr)
#define ptr_cast(intnum) reinterpret_cast<void*>(intnum)
#define ptr_caste(intnum, type) reinterpret_cast<type*>(ptr_cast(intnum))

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

static sys_memory_t mem_id; 
static sys_addr_t addr[4];

int main() {

	static u32 ret64 = 0, ret1m = 0; 

	// Try default flags with 64k allocation size
	ret64 = sys_memory_allocate(0x10000, 0x0, &addr[0]);

	// Try default flags with 1mb allocation size
	ret1m = sys_memory_allocate(0x100000, 0x0, &addr[1]);

	printf("ret64=0x%x, ret1m=0x%x, addr1=0x%x, addr2=0x%x\n", ret64, ret1m, addr[0], addr[1]);

    printf("sample finished.\n");

    return 0;
}