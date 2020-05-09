#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <cstring>
#include <math.h>
#include <float.h>

#include "../spu_header.h"


int main(u64 count)
{
	count++; // Never write zero

	if (count % 2)
	{
		while (count >= 7)
		{
			// Don't exit, wait for external termination
		}

		sys_spu_thread_group_exit(count);
	}

	return static_cast<int>(count);
}