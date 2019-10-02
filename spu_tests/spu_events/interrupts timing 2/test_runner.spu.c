#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	volatile uint32_t* pc = (void*)0;
	register uint32_t r92 __asm__ ("92");
	(void)arg1;
	(void)arg2;
	(void)arg4;

	//char scratchBuf[8192] __attribute__((aligned(128))) = {0};
	//const uint32_t* start = (void*)0; 
  volatile char failedBuf[8192] __attribute__((aligned(128))) = {0};
	*pc = 0x3f00;

	spu_printf("waiting for an error ,else stall\n");
	 spu_writech(SPU_WrEventMask, 0);
		 spu_ienable();
	spu_writech(SPU_WrDec, 0);
	__asm__ volatile ("ilh $91 , 0x2");
	spu_writech(SPU_WrEventMask, 0x20);
   __asm__ volatile ("label:");
	__asm__ volatile ("brnz $91, label");
		__asm__ volatile ("brz $91, label");
   // __asm__ volatile ("stopd $2, $2, $2");
	//__asm__ volatile ("stopd $2, $2, $2");
  //  __asm__ volatile ("stopd $2, $2, $2");
  //  __asm__ volatile ("stopd $2, $2, $2");
	spu_printf("done\n"); // never occures!

	//spu_readch(SPU_RdEventStat);

   
		//__asm__ volatile ("stopd $2, $2, $2");
	sys_spu_thread_exit(0);
	return 0;
}

// result - interrupt fired, spe checks after every instruction if an interrupt condition is met and 