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

static u32 threads = 0;
static u32 save_stack[3];
register u64 stack asm ("1");

void threadEntry(u64 number) {
	save_stack[number] = stack; 
	sys_ppu_thread_stack_t sp;
	sys_ppu_thread_get_stack_information(&sp);
	printf("thread:0x%x stack addr=%x current stack offset=0x%x\n", number, sp.pst_addr, (sp.pst_addr + sp.pst_size) - save_stack[number]);
	cellAtomicDecr32(&threads);
	sys_ppu_thread_exit(0);
}

int main() {
	save_stack[0] = stack;
	sys_ppu_thread_stack_t sp;
	sys_ppu_thread_get_stack_information(&sp);
	printf("thread:0 stack addr=%x current stack offset=0x%x\n", sp.pst_addr, (sp.pst_addr + sp.pst_size) - save_stack[0]);
	sys_timer_usleep(3000);
	static sys_ppu_thread_t m_tid1; threads = 2;
	sys_ppu_thread_create(&m_tid1,threadEntry,1,1002, 0x10000,0,"t");
	sys_ppu_thread_create(&m_tid1,threadEntry,2,1002, 0x10000,0,"t");

	while(threads) sys_timer_usleep(40000);
	
    printf("sample finished.\n");

    return 0;
}