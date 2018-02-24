#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

inline void cell_ok()
{
    static uint32_t ok = 0;
    spu_printf("ok %x\n", ok++);
}
int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
       
    (void)arg1;
    (void)arg2;
	(void)arg4;
    //char scratchBuf[8192] __attribute__((aligned(128))) = {0};
    //const uint32_t* start = (void*)0; 


  while(spu_readchcnt(SPU_RdSigNotify1) == 0){}
  cell_ok();
  uint32_t addr = spu_readch(SPU_RdSigNotify1);
    uint32_t Buf[2048] __attribute__((aligned(128))) = {0};

    spu_printf("%x\n", addr);
    spu_writech(MFC_WrTagMask, -1);
    spu_writech(MFC_EAL, addr);
    spu_writech(MFC_LSA , (uint32_t)static_cast<void*>(&Buf[0]));
    spu_writech(MFC_Cmd, 0xd0);

  cell_ok();

    while (spu_readchcnt(MFC_RdAtomicStat) == 0){}
        spu_writech(MFC_EAL, 0);
    spu_writech(MFC_LSA , (uint32_t)static_cast<void*>(&Buf[0]));
    spu_writech(MFC_Cmd, 0xb4);
  cell_ok();

   spu_printf("%x\n",spu_readch(MFC_RdAtomicStat));
        //__asm__ volatile ("stopd $2, $2, $2");
    sys_spu_thread_exit(0);
	return 0;
}