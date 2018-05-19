#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main()
{
    register uint64_t stk asm ("1");
    register vec_int4 reg asm ("98");
    reg = (vec_int4){1, 2, 3, 4};
    asm volatile ("cwd $98, 0($1)");
    //si_rotqmbyi
    asm volatile ("rdch $98, $SPU_RdDec");
    spu_printf("stuff = 0x%x", spu_extract(reg, 1));
    sys_spu_thread_exit(-1);
	return 0;
}