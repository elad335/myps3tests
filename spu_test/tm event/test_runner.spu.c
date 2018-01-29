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

    spu_writech(MFC_TagID, 0);
    spu_writech(MFC_Cmd, 0xcc);
    spu_writech(MFC_WrTagMask, 3);
    spu_writech(MFC_WrTagUpdate, 2);  
    while (spu_readchcnt(MFC_RdTagStat) == 0){}

   spu_printf("%x\n xxx",spu_readch(MFC_RdTagStat));
        //__asm__ volatile ("stopd $2, $2, $2");
    sys_spu_thread_exit(0);
	return 0;
}