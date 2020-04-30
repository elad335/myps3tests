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
static u64 dst_time;
static u64 signal_ = 0;
int ret = 0;

// Thread IDs
sys_ppu_thread_t tid[4];

// Thread priorities
s32 prio[4] = {2, 2, 2, 2};

void waiter(u64 index)
{
	// Wait until all threads are created
	while (load_vol(signal_) == 0);

	while (load_vol(signal_) == 1)
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
	ENSURE_OK(sys_ppu_thread_create(&tid[0], &waiter, 0, prio[0], 0x100000, 1, "t0"));
	ENSURE_OK(sys_ppu_thread_create(&tid[1], &waiter, 1, prio[1], 0x100000, 1, "t1"));
	ENSURE_OK(sys_ppu_thread_create(&tid[2], &waiter, 2, prio[2], 0x100000, 1, "t2"));
	ENSURE_OK(sys_ppu_thread_create(&tid[3], &waiter, 3, prio[3], 0x100000, 1, "t3"));

	store_vol(signal_, 1);
	sys_timer_sleep(10);
	store_vol(signal_, 2);

	// Save counts
	const u32 m_counts[4] = 
	{
		load_vol(count[0 * 32]),
		load_vol(count[1 * 32]),
		load_vol(count[2 * 32]),
		load_vol(count[3 * 32])
	};

	// Release current thread and join 
	for (u64 i = 0; i < 4; i++)
	{
		ENSURE_VAL(sys_ppu_thread_join(tid[i], NULL), EFAULT);
	}

	printf("count0=0x%x, count1=0x%x, count2=0x%X, count3=0x%x\n"
	, m_counts[0], m_counts[1], m_counts[2], m_counts[3]);

	return 0;
}