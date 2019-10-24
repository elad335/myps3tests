#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <cell/dma.h>

#include "../spu_header.h"

int main(u64 cmd, u64 addr, u64 dont_exit)
{
	u32 to_write;
	if (cmd == 1)
	{
		to_write = spu_stat_signal1();
	}
	else if (cmd == 2)
	{
		to_write = spu_stat_in_mbox();
	}
	else
	{
		// Unreachable
		asm volatile ("stopd 0, 0, 0");
	}
	

	cellDmaPutUint32(to_write, addr, 0, 0, 0);

	if (dont_exit)
	{
		// Ternimate the thread from sys_spu_thread_group_terminate instead
		while (1);
	}

	//sys_spu_thread_group_exit(0);
	sys_spu_thread_exit(0);
	return 0;
}