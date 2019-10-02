#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	volatile uint32_t* pc = (void*)0;
	   
	(void)arg1;
	(void)arg2;
	(void)arg4;

	//char scratchBuf[8192] __attribute__((aligned(128))) = {0};
	//const uint32_t* start = (void*)0; 
  volatile char failedBuf[8192] __attribute__((aligned(128))) = {0};

	spu_printf("to stall or not to stall, thats the question\n");
	spu_writech(MFC_WrTagMask, 0);
	spu_writech(MFC_WrTagUpdate, 1/*1*/);
	spu_writech(SPU_WrEventMask, 0x1);
	while (spu_readchcnt(SPU_RdEventStat) == 0){
		 spu_printf("ugh\n");
		 spu_writech(MFC_WrTagMask, -1);
	}
	spu_printf("done\n");
	sys_spu_thread_exit(0);
	return 0;
}

// Result - didnt stall on type 2 requast but did on type 1, type 1 stall can be released when writing a non zero number to the tag mask.
// Type 0 didnt stall as expected