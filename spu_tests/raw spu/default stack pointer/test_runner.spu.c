#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main(void)
{
	//spu_printf("Raw memes thread\n");
	spu_writech(SPU_WrDec, -1); // one second delay
	si_sync(); si_syncc(); si_dsync();
	spu_readch(SPU_RdInMbox);
	asm ("nop;nop;nop;nop;nop;nop;nop");
	si_sync(); si_syncc(); si_dsync();
	while(1);
	return 0;
}