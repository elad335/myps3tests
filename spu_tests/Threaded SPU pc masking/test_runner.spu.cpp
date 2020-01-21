#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <cstring>
#include <math.h>
#include <float.h>

#include "../spu_header.h"

int main()
{
	spu_printf("interrupts state=0x%x", spu_readch(SPU_RdMachStat) & 1);

	return 0;
}