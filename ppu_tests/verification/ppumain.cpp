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
register u64 stack asm ("1");

static sys_lwmutex_t lwmutex[5];
static sys_lwmutex_attribute_t mutex_attr = { SYS_SYNC_PRIORITY, SYS_SYNC_RECURSIVE, "/0"};
static sys_lwcond_t lwcond[5];
static sys_lwcond_attribute_t cond_attr = { "/0" };
register uint32_t err asm ("3"); 

void threadNotify(u64 number) {

	while (true)
	{
		sys_timer_usleep(1000);
		sys_lwmutex_lock(&lwmutex[number], 0);
		if (sys_lwcond_signal_all(&lwcond[number]) != CELL_OK) printf("NOT CEL_OK");
		sys_lwmutex_unlock(&lwmutex[number]);
	}
	sys_ppu_thread_exit(0);
}

void threadWait(u64 number) {

	while (true)
	{
		sys_lwmutex_lock(&lwmutex[number], 0);
		//if (sys_lwcond_wait(&lwcond[number], 0) != CELL_OK) printf("NOT CELL_OK");
		{system_call_3(0x071, *ptr_caste(int_cast(&lwcond[number]) + 4, u32), *ptr_caste(int_cast(&lwmutex[number]) + 16, u32), 0);
		printf("error code=%x", err); }
		sys_lwmutex_unlock(&lwmutex[number]);
		sys_timer_usleep(600);
	}
	sys_ppu_thread_exit(0);
}

int main() {
	static sys_ppu_thread_t m_tid1;

	for (u32 i = 0; i < 1; i++){
		if (sys_lwmutex_create(&lwmutex[i], &mutex_attr) != CELL_OK) exit(-1);
		if (sys_lwcond_create(&lwcond[i], &lwmutex[i], &cond_attr) != CELL_OK) exit(-1);
		asm volatile ("eieio; sync");
		sys_ppu_thread_create(&m_tid1,threadWait,i,1002, 0x10000,0,"Waiter");
		sys_ppu_thread_create(&m_tid1,threadWait,i,1001, 0x10000,0,"Waiter");
		sys_ppu_thread_create(&m_tid1,threadNotify,i,1002, 0x10000,0,"Notifier");
	}

	while(true || threads) sys_timer_sleep(400);
	
    printf("sample finished.\n");

    return 0;
}