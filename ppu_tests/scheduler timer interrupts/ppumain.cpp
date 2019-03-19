#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/sys_time.h>
#include <functional>
#include <cell/atomic.h>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

static u32 count[4] = {0};
static volatile u64 dst_time;

int ret = 0;

static u64 _mftb()
{
	u64 ret;
	while (!(ret = __mftb()));
	return ret;
}

void waiter(u64 index)
{
	while (_mftb() < dst_time)
	{
		cellAtomicIncr32(&count[index]);
	}

	sys_ppu_thread_exit(0);
}


int main() 
{
	sys_ppu_thread_t tid[4];
	ERROR_CHECK_RET(sys_ppu_thread_create(&tid[0], &waiter, 0, 1001, 0x10000, 1, "t0"));
	ERROR_CHECK_RET(sys_ppu_thread_create(&tid[1], &waiter, 1, 1001, 0x10000, 1, "t1"));
	ERROR_CHECK_RET(sys_ppu_thread_create(&tid[2], &waiter, 2, 1001, 0x10000, 1, "t2"));
	ERROR_CHECK_RET(sys_ppu_thread_create(&tid[3], &waiter, 3, 1001, 0x10000, 1, "t3"));

	dst_time = _mftb() + (sys_time_get_timebase_frequency() * 10);
	sys_timer_sleep(10);

	printf("count0=0x%x, count1=0x%x, count2=0x%X, count3=0x%x\n", count[0], count[1], count[2], count[3]);

	for (u64 i = 0, ret = 0; i < 4; i++)
	{
		ERROR_CHECK_RET(sys_ppu_thread_join(tid[i], &ret));
	}

    return 0;
}