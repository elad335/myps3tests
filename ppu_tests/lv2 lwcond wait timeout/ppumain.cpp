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
SYS_PROCESS_PARAM(1000, 0x10000)

static u32 sig = 0;

#define GROUP_NUM 2
sys_lwmutex_t lwmutex[GROUP_NUM];
sys_lwmutex_attribute_t mutex_attr = { SYS_SYNC_PRIORITY, SYS_SYNC_NOT_RECURSIVE, "/0"};
sys_lwcond_t lwcond[GROUP_NUM];
sys_lwcond_attribute_t cond_attr = { "/0" };
sys_ppu_thread_t threads[GROUP_NUM];

inline bool test_lock(sys_lwmutex_t* mtx)
{
	if (lv2_lwmutex_trylock(mtx) == CELL_OK) 
	{
		lv2_lwmutex_unlock(mtx);
		return false;
	}
	return true;
}

//void threadNotify(u64 index)
//{
//	while (true)
//	{
//		// Commented 
//	}
//
//	cellAtomicDecr32(&sig); sys_ppu_thread_exit(0);
//}

void threadWait(u64 index) 
{
	while (true)
	{
		lv2_lwcond_wait(&lwcond[index], &lwmutex[index], 1000);
		printf("locking state:0x%x", test_lock(&lwmutex[index]));
		//sys_timer_usleep(200000);
		break;
	}
	cellAtomicDecr32(&sig); sys_ppu_thread_exit(0);
}

int main() 
{
	sys_ppu_thread_t dummy;
	for (u32 i = 0; i < GROUP_NUM; i++)
	{
		if (sys_lwmutex_create(&lwmutex[i], &mutex_attr) != CELL_OK) exit(-1);
		if (sys_lwcond_create(&lwcond[i], &lwmutex[i], &cond_attr) != CELL_OK) exit(-1);
		cellAtomicAdd32(&sig, 1);
		sys_ppu_thread_create(&threads[i],threadWait,i,1002, 0x10000,0,"Waiter");
	}

	while(load_vol(sig)) sys_timer_sleep(10);
	
	printf("sample finished.\n");

	return 0;
}