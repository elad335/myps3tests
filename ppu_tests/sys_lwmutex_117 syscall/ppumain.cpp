#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <functional>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_lwmutex_t lwmutex, invalid_mutex;
sys_lwmutex_attribute_t mutex_attr = { SYS_SYNC_RETRY, SYS_SYNC_NOT_RECURSIVE, "/0"};
sys_lwcond_t lwcond, invalid_cond;
sys_lwcond_attribute_t cond_attr = { "/0" };

inline u32 lv2_lwmutex_117(sys_lwmutex* mtx)
{
	system_call_1(117, *ptr_caste(int_cast(mtx) + 16, u32));
	return_to_user_prog(u32);
}

#define ERROR_CHECK_RET(x) if (x != CELL_OK) { printf("Fatl error!"); exit(-1); }

int ret = 0;

void waiter(u64)
{
	ret = lv2_lwmutex_lock(&lwmutex, 0);
	printf("Waiter: lwmutex_lock() returned 0x%x\n", ret);
	sys_ppu_thread_exit(0);
}

int main() 
{
	sys_ppu_thread_t main_tid, waiter_tid;
	sys_ppu_thread_get_id(&main_tid);

	ERROR_CHECK_RET(sys_lwmutex_create(&lwmutex, &mutex_attr));
	ERROR_CHECK_RET(sys_lwcond_create(&lwcond, &lwmutex, &cond_attr));

	ERROR_CHECK_RET(lv2_lwmutex_117(&lwmutex));
	printf("Main: lwmutex_117()\n");
	printf("Main: lwmutex_lock() returned 0x%x\n", lv2_lwmutex_lock(&lwmutex, 0));
	printf("Main: lwmutex_trylock() returned 0x%x\n", lv2_lwmutex_trylock(&lwmutex));

	ERROR_CHECK_RET(lv2_lwmutex_117(&lwmutex));
	ERROR_CHECK_RET(lv2_lwmutex_unlock(&lwmutex));
	printf("Main: lwmutex_117(), lwmutex_unlock()\n");
	printf("Main: lwmutex_lock() returned 0x%x\n", lv2_lwmutex_lock(&lwmutex, 0));

	ERROR_CHECK_RET(lv2_lwmutex_unlock(&lwmutex));
	ERROR_CHECK_RET(lv2_lwmutex_117(&lwmutex));
	printf("Main: lwmutex_unlock(), lwmutex_117()\n");
	printf("Main: lwmutex_lock() returned 0x%x\n", lv2_lwmutex_lock(&lwmutex, 0));

	ERROR_CHECK_RET(lv2_lwmutex_unlock(&lwmutex));
	ERROR_CHECK_RET(lv2_lwmutex_117(&lwmutex));
	printf("Main: lwmutex_unlock(), lwmutex_117()\n");
	printf("Main: lwmutex_trylock() returned 0x%x\n", lv2_lwmutex_trylock(&lwmutex));

	ERROR_CHECK_RET(lv2_lwmutex_117(&lwmutex));
	printf("Main: lwmutex_117()\n");
	printf("Main: lwmutex_trylock() returned 0x%x\n", lv2_lwmutex_trylock(&lwmutex));

	ERROR_CHECK_RET(sys_ppu_thread_create(&waiter_tid, &waiter, 0, 1001, 0x10000, 1, "Waiter"));

	// Wait for waiter to wait for lock (unsafe)
	sys_timer_sleep(5);

	lv2_lwmutex_117(&lwmutex);
	ERROR_CHECK_RET((sys_ppu_thread_join(waiter_tid, NULL) != EFAULT));

    return 0;
}