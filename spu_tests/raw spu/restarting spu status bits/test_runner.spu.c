#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main(void)
{
	//spu_printf("Raw memes thread\n");
	si_sync(); si_syncc(); si_dsync();
	si_sync(); si_syncc(); si_dsync();
	spu_writech(69, 0x7c00);
	__asm__ volatile ("nop;nop;nop;nop;nop;nop;nop;nop");
	__asm__ volatile ("nop;nop;nop;nop;nop;nop;nop;nop");
	si_sync(); si_syncc(); si_dsync();
	si_sync(); si_syncc(); si_dsync();
	si_sync(); si_syncc(); si_dsync();
	si_sync(); si_syncc(); si_dsync();
    spu_writech(SPU_WrDec, -1); // one second delay
	si_sync(); si_syncc(); si_dsync();
    spu_writech(SPU_WrOutMbox, spu_readch(SPU_RdDec));
	si_sync(); si_syncc(); si_dsync();
	//while(1);
	//spu_writech(SPU_RdEventStat, 3);
	//asm ("stop 0x3");
	return 0;
}