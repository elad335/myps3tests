#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main(void)
{
	spu_writech(SPU_WrOutIntrMbox, 0); // Trigger interrupt
	return 0;
}
