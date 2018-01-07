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

    spu_printf("waiting 5 seconds\n");
    spu_writech(SPU_WrDec, 400000000);
    spu_writech(SPU_WrEventMask, 0x20);
    while (spu_readchcnt(SPU_RdEventStat) == 0){}
    spu_writech(SPU_WrEventAck, 0x20);
    spu_readch(SPU_RdEventStat);
    while (spu_readchcnt(SPU_RdEventStat) == 0){}
    spu_printf("done\n"); // Never occur , not even after one minute (decrementer max capacity)

        //__asm__ volatile ("stopd $2, $2, $2");
    sys_spu_thread_exit(0);
	return 0;
}

// result - stall, after the tm event is signalled, only a new write to the decrementer can make tm event occur again.