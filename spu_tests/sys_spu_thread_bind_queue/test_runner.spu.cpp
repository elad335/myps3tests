#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <cstring>
#include <math.h>
#include <float.h>

#include "../spu_header.h"

int main(u64 spu_q_number)
{
	u32 t1 = ~0, t2 = t1, t3 = t1, ret = t1;
	ret = sys_spu_thread_receive_event(spu_q_number, &t1, &t2, &t3);
	spu_printf("SPU: sys_spu_thread_receive_event(error 0x%x, data = {0x%x, 0x%x, 0x%x})\n", ret, t1, t2, t3);
	sys_spu_thread_exit(0);
	return 0;
}