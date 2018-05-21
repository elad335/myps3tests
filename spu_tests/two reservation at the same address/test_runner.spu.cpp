#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>

#define u32 uint32_t
#define u64 uint64_t
#define uptr uintptr_t


int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    unsigned int* pc = 0;
    *pc = 0x3f45;
    while(spu_readchcnt(SPU_RdSigNotify1) == 0){}
    spu_printf("ok\n");
    char arr[8192] __attribute__((aligned(256))) = {0};
    char* ptr = &arr[0];
    uint32_t addr = spu_readch(SPU_RdSigNotify1);

    asm volatile ("nop;syncc;sync;dsync");

    spu_writech(SPU_WrEventMask, 0x400);
    if (spu_readchcnt(SPU_RdEventStat)) spu_writech(SPU_WrEventAck,(spu_readch(SPU_RdEventStat)));
    asm volatile ("nop;syncc;sync;dsync");

    spu_writech(MFC_EAL, addr);
    spu_writech(MFC_LSA , (uptr)ptr);
    spu_writech(MFC_TagID, 0);
    spu_writech(MFC_Size, 0x80);
    spu_writech(MFC_Cmd, 0xD0);
    spu_readch(MFC_RdAtomicStat);
    asm volatile ("nop;syncc;sync;dsync;nop;syncc;sync;dsync;nop;syncc;sync;dsync;nop;syncc;sync;dsync;nop;syncc;sync;dsync;nop;syncc;sync;dsync");

    spu_writech(MFC_EAL, addr);
    spu_writech(MFC_LSA , (uptr)ptr); // same address
    spu_writech(MFC_TagID, 0);
    spu_writech(MFC_Size, 0x80);
    spu_writech(MFC_Cmd, 0xD0);
    spu_readch(MFC_RdAtomicStat);
    asm volatile ("nop;syncc;sync;dsync;nop;syncc;sync;dsync;nop;syncc;sync;dsync;nop;syncc;sync;dsync;nop;syncc;sync;dsync;nop;syncc;sync;dsync");

    spu_printf("events channel count: {0x%x} \n", spu_readchcnt(SPU_RdEventStat));

    sys_spu_thread_exit(0);
	return 0;
}