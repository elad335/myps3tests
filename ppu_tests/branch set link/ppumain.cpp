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

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

void threadEntry(u64 number) 
{
	register u64 reg31 asm ("31");

	asm volatile (
	"li 31, 0;"
	"mtlr 31;"
	"mtocrf 0x4, 0;"
	"bcl 12, 23, 0x0;"
	"mflr 31;");
	
	printf("r31: 0x%08x\n", reg31);
	sys_ppu_thread_exit(0);
}

int main()
{
	static sys_ppu_thread_t tid;
	sys_ppu_thread_create(&tid,threadEntry,1,1002, 0x10000,1,"t");

	u64 status;
	sys_ppu_thread_join(tid, &status);

	sys_timer_usleep(500);
    printf("sample finished.\n");

    return 0;
}