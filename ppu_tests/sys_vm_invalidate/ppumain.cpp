#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_syscache.h>
#include <cell/sysmodule.h>
#include <cell/atomic.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <sys/vm.h> 
#include <functional>

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int32_t s32;

#define int_cast(addr) reinterpret_cast<uptr>(addr)
#define ptr_cast(intnum) reinterpret_cast<void*>(intnum)
#define ptr_caste(intnum, type) reinterpret_cast<type*>(ptr_cast(intnum))

u32 addr = 0;

u32 read_buf()
{
	return ((*ptr_caste(addr, volatile u8)) << 24) |
	((*ptr_caste(addr + 0x4000, volatile u8)) << 16) |
	((*ptr_caste(addr + 0x8000, volatile u8)) << 8) |
	((*ptr_caste(addr + 0xc000, volatile u8)) << 0);
}

int main()
{
	sys_vm_memory_map(0x2000000, 0x100000, -1, 0x200, 1, &addr);

	sys_vm_touch(addr, 0x100000);
	sys_vm_sync(addr, 0x100000);

	printf("Values = 0x%x\n", read_buf());
	memset((void*)addr, 0xFE, 0x10000);

	sys_vm_touch(addr, 0x100000);
	sys_vm_sync(addr, 0x100000);

	printf("Values = 0x%x\n", read_buf());
	memset((void*)addr, 0xFE, 0x10000);

	sys_vm_invalidate(addr, 0x100000);
	sys_vm_sync(addr, 0x100000);

	printf("Values = 0x%x\n", read_buf());

    printf("sample finished.\n");
    return 0;
}