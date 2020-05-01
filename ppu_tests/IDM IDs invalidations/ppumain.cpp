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
#include <sysutil/sysutil_sysparam.h>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1, 0x100000)

sys_ppu_thread_t main_tid;
u32 signal_exit = 0;

void threadEntry(u64 stay)
{
	while (stay)
	{
		sys_timer_sleep(1);
	}

	sys_ppu_thread_exit(0);
}

int main() {

	sys_mutex_attribute_t mtxAttr;
	sys_mutex_attribute_initialize(mtxAttr);
	sys_mutex_t MutexId[512];
	// Make Mutex
	for (u32 i = 0; i < 512; i++)
	{
		ENSURE_OK(sys_mutex_create(&MutexId[i], &mtxAttr));
		ENSURE_OK(sys_mutex_destroy(MutexId[i]));
	}

	sys_ppu_thread_t m_tid[512];

	for (u32 i = 0; i < 512; i++)
	{
		ENSURE_OK(sys_ppu_thread_create(&m_tid[i],threadEntry,0,0, 0x100000,1, NULL));
		ENSURE_VAL(sys_ppu_thread_join(m_tid[i], NULL), EFAULT);
	}


	printf("Mutex IDs:\n");
	print_obj(MutexId);
	printf("PPU IDs:\n");
	print_obj(m_tid);
	printf("sample finished.\n");
	ENSURE_OK(sys_ppu_thread_create(&m_tid[0],threadEntry,1,0, 0x100000,1, NULL));
	sys_timer_usleep(200);
	sys_ppu_thread_exit(1);
	return 0;
}