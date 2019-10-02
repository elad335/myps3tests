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
SYS_PROCESS_PARAM(100000, 0x10000)

int main() {

	u64 main_tid;
	sys_ppu_thread_get_id(&main_tid);

	int prio;
	sys_ppu_thread_get_priority(main_tid, &prio);

	printf("prio: %d\n",prio);
	printf("sample finished.\n");

	return 0;
}