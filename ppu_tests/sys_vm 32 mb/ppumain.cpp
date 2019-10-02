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
#include <sys/vm.h> 
#include <functional>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

register u64 stack asm ("1");

int main() {

	static u32 addr = 1;
	sys_vm_memory_map(0x2000000, 0x100000, -1, 0x400, 1, &addr);
	sys_vm_memory_map(0x2000000, 0x100000, -1, 0x400, 1, &addr);
	
	printf("sample finished. addr=0x%x\n", addr);

	return 0;
}