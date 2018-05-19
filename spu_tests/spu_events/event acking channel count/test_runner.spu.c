#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    //volatile u32* pc = (void*)0;
       
    (void)arg1;
    (void)arg2;
    arg3;
	(void)arg4;

    //char scratchBuf[8192] __attribute__((aligned(128))) = {0};
    //const uint32_t* start = (void*)0; 
  volatile char failedBuf[8192] __attribute__((aligned(128))) = {0};

    spu_printf("checking event stat count after an ack\n");
    spu_writech(SPU_WrDec, 160000000);
    spu_writech(SPU_WrEventMask, 0x20);
    while (spu_readchcnt(SPU_RdEventStat) == 0){}
    spu_writech(SPU_WrEventAck, 0x20);
    spu_printf("count = 0x%x\n", spu_readchcnt(SPU_RdEventStat));
    spu_printf("done\n");

    //spu_readch(SPU_RdEventStat);

   
        //__asm__ volatile ("stopd $2, $2, $2");
    sys_spu_thread_exit(0);
	return 0;
}
// result , an event ack doesnt decrement event stat count no matter what