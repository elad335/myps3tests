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
#include <cell/atomic.h>
#include <memory>

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define int_cast(addr) reinterpret_cast<uptr>(addr)
#define ptr_cast(intnum) reinterpret_cast<void*>(intnum)
#define ptr_caste(intnum, type) reinterpret_cast<type*>(ptr_cast(intnum))
#define sync() asm volatile ("eieio;sync")

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

static sys_addr_t addr;
static u32* ea;

int main() {

	// Allocate 1mb aligned memory block
	sys_memory_allocate(0x100000, 0x400, &addr);
	asm volatile ("twi 0x10, 3, 0");

	*ptr_caste(addr, u32) = 0x01020304; // Store value to check later

	ea = ptr_caste(addr + 1, u32); // Unaligned pointer

	sync(); // Paranoidic memory fence
	u32 old = __lwarx(ea);
	sync();
	bool success = __stwcx(ea, 0x04030201) != 0;
	sync();


    printf("old value=0x%x, new value=0x%x, success=%s\nsample finished.\n", old, *ptr_caste(addr, u32), success ? "true" : "false");

    return 0;
}