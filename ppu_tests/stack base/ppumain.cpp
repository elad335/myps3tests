#include <stdio.h>
#include <stdlib.h>

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

static u32 threads = 0;
static u32 save_stack[3];

void threadEntry(u64 number) {
	register u64 stack asm ("1");
	save_stack[number]= stack; 
	printf("thread:0x%x stack addr=0x%llx\n", number, save_stack[number]);
	cellAtomicDecr32(&threads);
	sys_ppu_thread_exit(0);
}

int main() {
	register u64 stack asm ("1");
	save_stack[0] = stack;
	printf("thread:0 stack addr=0x%x\n", save_stack[0]);
	sys_timer_usleep(3000);
	static sys_ppu_thread_t m_tid1; threads = 2;
	sys_ppu_thread_create(&m_tid1,threadEntry,1,1002, 0x4000,0,"t");
	sys_ppu_thread_create(&m_tid1,threadEntry,2,1002, 0x4000,0,"t");

	while(threads) sys_timer_usleep(40000);
	
    printf("sample finished.\n");

    return 0;
}