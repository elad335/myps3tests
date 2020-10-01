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

void threadEntry(u64)
{
	// Unreachable, did not invoke interrupt
	store_vol(*(u8*)(NULL), 1);
}

int main() {

	sys_ppu_thread_t id;
	u64 i = 0;
	for (s32 err = 0;; i++)
	{
		// Create an interrupt thread with 4k stack
		err = sys_ppu_thread_create(&id,threadEntry,0,3071,4096,SYS_PPU_THREAD_CREATE_INTERRUPT,"thread");

		if (err != CELL_OK)
		{
			if (err != EAGAIN)
			{
				printf("Hard failure: Error 0x%x\n", err);
				return err;
			}
			break;
		}
	}

	printf("sample finished. Created PPU thread: %u\n", i);

	return 0;
}