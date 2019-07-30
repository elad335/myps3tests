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
#include <functional>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x100000)

static u32 signal_exit = 0;
static u32 threads = 0;

void threadEntry(u64) {
	register u64 stk asm ("1");
	printf("stackptr=0x%x\n", stk);
	while(!signal_exit) sys_timer_usleep(30000);
	threads--;
	sys_ppu_thread_exit(0);
}

int main() {

	int result;
	sys_ppu_thread_t m_tid1;

	while(!(result = sys_ppu_thread_create(&m_tid1,threadEntry,0,1002, 0x1000000,0,"t")))
	{
		threads++;
	}	

    printf("threads created:0x%x, error_code=0x%x\n",threads, result);
	signal_exit = 1;

	while(threads) sys_timer_usleep(40000);
	
    printf("sample finished.\n");

    return 0;
}