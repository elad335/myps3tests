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
    uint32_t Buf_O[4] __attribute__((aligned(128))) = {0x80000100, addr , 0x80000100 , addr};
        uint32_t exception_list[2] __attribute__((aligned(128))) = {0x00000500, addr, 0x00000500, addr, 0x00000500, addr};
    uint32_t Buf[2048] __attribute__((aligned(128))) = {0};

  while (spu_readchcnt(MFC_Cmd) > 2/*3*/)
  {
        spu_writech(MFC_EAL, (uint32_t)static_cast<void*>(&Buf_O[0]));
        spu_writech(MFC_LSA , (uint32_t)static_cast<void*>(&Buf[0]));
        spu_writech(MFC_TagID, 0);
        spu_writech(MFC_Size, 16);
        spu_writech(MFC_Cmd, 0x44);
  }
  {
        spu_writech(MFC_EAL, (uint32_t)static_cast<void*>(&Buf_O[0]));
        spu_writech(MFC_LSA , (uint32_t)static_cast<void*>(&Buf[0]));
        spu_writech(MFC_TagID, 2);
        spu_writech(MFC_Size, 16);
        spu_writech(MFC_Cmd, 0x44);
  }
  {
        spu_writech(MFC_EAL, (uint32_t)static_cast<void*>(&exception_list[0]));
        spu_writech(MFC_LSA , (uint32_t)static_cast<void*>(&Buf[0]));
        spu_writech(MFC_TagID, 3);
        spu_writech(MFC_Size, 24);
        spu_writech(MFC_Cmd, 0x44);
  }
    spu_printf("ok2\n");
    spu_printf("%x\n", addr);

    spu_printf("notice\n");
    spu_writech(MFC_WrTagMask, -1);
    spu_writech(MFC_EAL, addr);
    spu_writech(MFC_LSA , (uint32_t)static_cast<void*>(&Buf[0]));
    spu_writech(MFC_Cmd, 0xd0);

    spu_printf("ok3\n");
    wait:
    while (spu_readchcnt(MFC_RdListStallStat) == 0){}

    spu_printf("ok4\n");
    if (spu_readch(MFC_RdListStallStat) & 0x4)
    {
        spu_writech(MFC_WrListStallAck, 0x4);
    }
    else
    {
        goto wait;
    }
    while (spu_readchcnt(MFC_RdAtomicStat) == 0){}

   spu_printf("%x\n",spu_readch(MFC_RdAtomicStat));
        //__asm__ volatile ("stopd $2, $2, $2");
    sys_spu_thread_exit(0);
	return 0;
}