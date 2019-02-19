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
sys_lwmutex_attribute_t mutex_attr = { SYS_SYNC_PRIORITY, SYS_SYNC_NOT_RECURSIVE, "/0"};
sys_lwcond_t lwcond, invalid_cond;
sys_lwcond_attribute_t cond_attr = { "/0" };
sys_ppu_thread_t threads;

inline bool test_lock(sys_lwmutex_t* mtx)
{
	if (lv2_lwmutex_trylock(mtx) == CELL_OK) 
	{
		lv2_lwmutex_unlock(mtx);
		return false;
	}
	return true;
}

int main() 
{
	sys_ppu_thread_t main_tid;
	sys_ppu_thread_get_id(&main_tid);

	if (sys_lwmutex_create(&lwmutex, &mutex_attr)) exit(-1);
	if (sys_lwcond_create(&lwcond, &lwmutex, &cond_attr)) exit(-1);

	int ret;

	ret = lv2_lwcond_signal(&lwcond, &lwmutex, 0, 1);
	printf("lwcond_signal(null id, mode 1) returned 0x%x\n", ret);

	lv2_lwmutex_unlock(&lwmutex);
	ret = lv2_lwcond_signal(&lwcond, &lwmutex, main_tid, 2);
	printf("lwcond_signal(main_thread, mode 2) returned 0x%x\n", ret);

	ret = lv2_lwcond_signal(&lwcond, &lwmutex, -1, 2);
	printf("lwcond_signal(-1 id, mode 2) returned 0x%x\n", ret);

	lv2_lwmutex_lock(&lwmutex, 0);
	ret = lv2_lwcond_signal(&lwcond, &lwmutex, main_tid, 3);
	printf("lwcond_signal(main_thread, mode 3) returned 0x%x\n", ret);
	ret = lv2_lwcond_signal(&lwcond, &lwmutex, -1, 3);
	printf("lwcond_signal(-1 id, mode 3) returned 0x%x\n", ret);

	lv2_lwmutex_unlock(&lwmutex);
	ret = lv2_lwcond_signal_all(&lwcond, &invalid_mutex, 2);
	printf("lwcond_signal_all(invalid mtx, mode 2) returned 0x%x\n", ret);

    return 0;
}