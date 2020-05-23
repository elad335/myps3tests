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
#include <sys/spu_thread_group.h> 
#include <functional>
#include <sysutil/sysutil_sysparam.h>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1, 0x100000)

sys_ppu_thread_t main_tid;
u32 signal_exit = 0;

void threadEntry(u64)
{
	sys_ppu_thread_exit(0);
}

int main()
{
	sys_mutex_attribute_t mtxAttr;
	sys_mutex_attribute_initialize(mtxAttr);
	sys_mutex_t MutexId[512];

	// Make Mutex
	for (u32 i = 0; i < 512; i++)
	{
		ENSURE_OK(sys_mutex_create(&MutexId[i], &mtxAttr));

		if (i != 0)
		{
			ENSURE_OK(sys_mutex_destroy(MutexId[i]));
		}
	}

	ENSURE_OK(sys_mutex_destroy(MutexId[0]));

	sys_ppu_thread_t PuId[512];

	for (u32 i = 0; i < 512; i++)
	{
		ENSURE_OK(sys_ppu_thread_create(&PuId[i],threadEntry,0,0, 0x100000,1, NULL));

		if (i != 0)
		{
			ENSURE_VAL(sys_ppu_thread_join(PuId[i], NULL), EFAULT);
		}
	}

	ENSURE_VAL(sys_ppu_thread_join(PuId[0], NULL), EFAULT);

	sys_spu_thread_group_attribute_t grp_attr;
	sys_spu_thread_group_attribute_initialize(grp_attr);

	sys_spu_thread_group_t GrpId[512];

	// Make Mutex
	for (u32 i = 0; i < 512; i++)
	{
		ENSURE_OK(sys_spu_thread_group_create(&GrpId[i], 1, 100, &grp_attr));

		if (i != 0)
		{
			ENSURE_OK(sys_spu_thread_group_destroy(GrpId[i]));
		}
	}

	ENSURE_OK(sys_spu_thread_group_destroy(GrpId[0]));

	printf("Mutex IDs:\n");
	print_obj(MutexId);
	printf("PPU IDs:\n");
	print_obj(PuId);
	printf("SPU TG IDs:\n");
	print_obj(GrpId);
	printf("sample finished.\n");
	return 0;
}