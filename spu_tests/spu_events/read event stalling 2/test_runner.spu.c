#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	   
	(void)arg1;
	(void)arg2;
	(void)arg4;

	//char scratchBuf[8192] __attribute__((aligned(128))) = {0};
	//const uint32_t* start = (void*)0; 
  volatile char failedBuf[8192] __attribute__((aligned(128))) = {0};
	spu_printf("check stalling\n");
	spu_ack_events();
	 spu_writech(SPU_WrEventMask, 1);
	spu_writech(SPU_WrEventAck, 1);
	spu_writech(MFC_WrTagUpdate, 0);
	while (spu_readchcnt(SPU_RdEventStat) == 0){}
	//spu_readch(SPU_RdEventStat);
	spu_printf("didnt stall%x\n", spu_readch(SPU_RdEventStat));

	//spu_readch(SPU_RdEventStat);

   
		//__asm__ volatile ("stopd $2, $2, $2");
	sys_spu_thread_exit(0);
	return 0;
}

// result - didnt stall, means that spe checks if there are events that can release the deadlock if event stat count is 0 , if no events are able to release the deadlock, spe abort and continues normally.