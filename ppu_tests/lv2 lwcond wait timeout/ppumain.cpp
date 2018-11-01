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
#define mfence() asm volatile ("eieio;sync")

static u32 sig = 0;
register u32 error asm ("3"); 

#define GROUP_NUM 2
sys_lwmutex_t lwmutex[GROUP_NUM];
sys_lwmutex_attribute_t mutex_attr = { SYS_SYNC_PRIORITY, SYS_SYNC_NOT_RECURSIVE, "/0"};
sys_lwcond_t lwcond[GROUP_NUM];
sys_lwcond_attribute_t cond_attr = { "/0" };
sys_ppu_thread_t threads[GROUP_NUM];

inline u32 _sys_lwcond_wait(sys_lwcond_t* lwc, sys_lwmutex_t* mtx, u64 timeout)
{
	{system_call_3(0x071, *ptr_caste(int_cast(lwc) + 4, u32), *ptr_caste(int_cast(mtx) + 16, u32), timeout);}
	return error;
}

inline u32 _sys_lwcond_signal_all(sys_lwcond_t* lwc, sys_lwmutex_t* mtx)
{
	{system_call_3(0x074, *ptr_caste(int_cast(lwc) + 4, u32), *ptr_caste(int_cast(mtx) + 16, u32), 1);}
	return error;
}

inline u32 _sys_lwmutex_lock(sys_lwmutex_t* mtx, u64 timeout)
{
	{system_call_2(0x061, *ptr_caste(int_cast(mtx) + 16, u32), timeout);}
	return error;
}

inline u32 _sys_lwmutex_trylock(sys_lwmutex_t* mtx)
{
	{system_call_1(0x063, *ptr_caste(int_cast(mtx) + 16, u32));}
	return error;
}

inline u32 _sys_lwmutex_unlock(sys_lwmutex_t* mtx)
{
	{system_call_1(0x062, *ptr_caste(int_cast(mtx) + 16, u32));}
	return error;
}

inline bool test_lock(sys_lwmutex_t* mtx)
{
	if (_sys_lwmutex_trylock(mtx) == CELL_OK) 
	{
		_sys_lwmutex_unlock(mtx);
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
		_sys_lwcond_wait(&lwcond[index], &lwmutex[index], 1000);
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
		cellAtomicAdd32(&sig, 1); mfence();
		sys_ppu_thread_create(&threads[i],threadWait,i,1002, 0x10000,0,"Waiter");
	}

	while(sig) sys_timer_sleep(10);
	
    printf("sample finished.\n");

    return 0;
}