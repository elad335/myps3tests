#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main(void)
{
	//spu_printf("Raw memes thread\n");
	si_sync(); si_syncc(); si_dsync();
	while(1);
	//spu_writech(SPU_RdEventStat, 3);
	//si_stop(0x3fff);
	return 0;
}