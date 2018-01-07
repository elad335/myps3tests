#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    /*also checked with irete, results at the end of the file*/
    volatile uint32_t* pc = (void*)0;
    *pc = 0x35400000; // iret
       
    (void)arg1;
    (void)arg2;
	(void)arg4;

    //char scratchBuf[8192] __attribute__((aligned(128))) = {0};
    //const uint32_t* start = (void*)0; 
  volatile char failedBuf[8192] __attribute__((aligned(128))) = {0};

    spu_printf("waiting for one second\n");
    spu_writech(SPU_WrDec, 80000000);
    spu_writech(SPU_WrEventMask, 0x20);
    spu_ienable();
    while (spu_readchcnt(SPU_RdEventStat) == 0){}
    spu_printf("done\n");

    //spu_readch(SPU_RdEventStat);

   
        //__asm__ volatile ("stopd $2, $2, $2");
    sys_spu_thread_exit(0);
	return 0;
}
// result - after setting interrupts to true ,checks for interrupts are made regardless of a *change* of event stat count and that interrupts checks occur after every instruction.
// also , no change to event stat count when an interrupt fired and if one enables interrupts at the end of interrupts handler and event stat count is 1 , the handler calls himself again until a decrement of event stat count.  