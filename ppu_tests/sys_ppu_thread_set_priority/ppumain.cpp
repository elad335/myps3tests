#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/sys_time.h>
#include <functional>
#include <cell/atomic.h>
#include <np.h>
#include <sys/spinlock.h>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1, 0x100000)

// Provide one variable per cache line
static u32 count[4 * 32] __attribute((aligned(128))) = {0};
static volatile u64 dst_time;
static volatile u64 signal_ = 0;
int ret = 0;

typedef volatile u32 vu32;

// Thread IDs
sys_ppu_thread_t tid[4];

// Thread priorities
s32 prio[4] = {2, 2, 2, 2};

void waiter(u64 index)
{
	// Wait until all threads are created
	while (signal_ == 0);

	while (signal_ == 1)
	{
		cellAtomicIncr32(&count[index * 32]);

		for (u32 i = 0 ; i < 4; i++)
		{
			if (i != index) 
			{
				sys_ppu_thread_set_priority(tid[i], prio[i]);
			}
		}
	}

	sys_ppu_thread_exit(0);
}


int main() 
{
	ERROR_CHECK_RET(sys_ppu_thread_create(&tid[0], &waiter, 0, prio[0], 0x100000, 1, "t0"));
	ERROR_CHECK_RET(sys_ppu_thread_create(&tid[1], &waiter, 1, prio[1], 0x100000, 1, "t1"));
	ERROR_CHECK_RET(sys_ppu_thread_create(&tid[2], &waiter, 2, prio[2], 0x100000, 1, "t2"));
	ERROR_CHECK_RET(sys_ppu_thread_create(&tid[3], &waiter, 3, prio[3], 0x100000, 1, "t3"));

	signal_ = 1;
	sys_timer_sleep(10);
	signal_ = 2;

	// Save counts
	const u32 m_counts[4] = { (vu32&)count[0 * 32], (vu32&)count[1 * 32], (vu32&)count[2 * 32], (vu32&)count[3 * 32] };

	// Release current thread and join 
	for (u64 i = 0, ret = 0; i < 4; i++)
	{
		ERROR_CHECK_RET(sys_ppu_thread_join(tid[i], &ret));
	}

	printf("count0=0x%x, count1=0x%x, count2=0x%X, count3=0x%x\n"
	, m_counts[0], m_counts[1], m_counts[2], m_counts[3]);

    return 0;
}