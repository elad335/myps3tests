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


  while(spu_readchcnt(SPU_RdSigNotify1) == 0){}
  spu_printf("ok\n");
  uint32_t addr = spu_readch(SPU_RdSigNotify1);
    uint32_t Buf_O[4] __attribute__((aligned(128))) = {0x00000100, addr , 0x00000100 , addr};
    uint32_t Buf[2048] __attribute__((aligned(128))) = {0};

    *(char*)((uint32_t)(&Buf[0]) | (0x1 << 31)) = 0;
    spu_printf("ok2\n");
    spu_printf("%x\n", addr);

    while (spu_readchcnt(MFC_Cmd) == 0){}
   spu_printf("hmm2\n");
        //__asm__ volatile ("stopd $2, $2, $2");
    sys_spu_thread_exit(0);
	return 0;
}