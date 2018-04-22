#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    unsigned int* pc = 0;
    *pc = 0x3f45;
    while(spu_readchcnt(SPU_RdSigNotify1) == 0){}
    spu_printf("ok\n");
    char Buf[8192] __attribute__((aligned(128))) = {0};
    uint32_t addr = spu_readch(SPU_RdSigNotify1);
      spu_writech(SPU_WrEventMask, 0x2);
    {volatile unsigned int i = 0;
    while (spu_readchcnt(MFC_Cmd) > 0/*3*/)
    {
        spu_writech(MFC_EAL, addr);
        spu_writech(MFC_LSA , reinterpret_cast<uintptr_t>(&Buf));
        spu_writech(MFC_TagID, i);
        //spu_writech(MFC_Size, 16);
        spu_writech(MFC_Cmd, 0x20);
        ++i;
    } i = 2000; while(i--) asm volatile ("nop;syncc;dsync");}


    while (spu_readchcnt(MFC_Cmd) == 0){}
        spu_writech(MFC_EAL, 1);
        spu_writech(MFC_LSA , 0);
        spu_writech(MFC_TagID, 0);
        spu_writech(MFC_Size, 16);
        spu_writech(MFC_Cmd, 0x20);
    
    spu_writech(MFC_WrTagMask, -1);
    spu_writech(MFC_WrTagUpdate, 0);
    spu_printf("the tag status is : %x", spu_readch(MFC_RdTagStat));

    //sys_spu_thread_exit(0);
	return 0;
}