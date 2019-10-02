#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>



int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	   
	(void)arg1;
	(void)arg2;
	(void)arg4;

	char scratchBuf[8192] __attribute__((aligned(128))) = {0};
	//const uint32_t* start = (void*)0; 

	spu_printf("waiting 5 seconds\n");
	//spu_ienable();
	spu_writech(SPU_WrDec, 40000000);
	spu_writech(SPU_WrEventMask, 0x20);
	while(1){};
  //spu_readch(SPU_RdEventStat);
	  __asm__ volatile ("nop $127");
	spu_printf("done\n");

	//spu_readch(SPU_RdEventStat);

   
		//__asm__ volatile ("stopd $2, $2, $2");
	sys_spu_thread_exit(0);
	return 0;
}