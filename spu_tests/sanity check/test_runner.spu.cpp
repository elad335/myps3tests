#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>

#define u32 uint32_t
#define u64 uint64_t
#define uptr uintptr_t

inline void sync() { asm volatile ("lnop;syncc;sync;dsync;syncc;sync;dsync;syncc;sync;dsync"); };

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    unsigned int* pc = 0;
    *pc = 0x3f45;
    while(spu_readchcnt(SPU_RdSigNotify1) == 0){}
    spu_printf("ok\n");
    char arr[8192] __attribute__((aligned(256))) = {0};
    char* ptr = &arr[0];
    uint32_t addr = spu_readch(SPU_RdSigNotify1);
    spu_printf("ok\n");
    sync();

    spu_writech(SPU_WrEventMask, 0x400); // LR event
    sync();
    if (spu_readchcnt(SPU_RdEventStat)) 
    {
        spu_writech(SPU_WrEventAck, spu_readch(SPU_RdEventStat)); // if count is 1, make it 0 and ack existing events
    }
    sync();
    spu_printf("ok\n");

    spu_writech(MFC_EAL, addr);
    spu_writech(MFC_LSA , (uptr)ptr);
    spu_writech(MFC_TagID, 0);
    spu_writech(MFC_Size, 0x80);
    spu_writech(MFC_Cmd, 0xD0); // GETLLAR
    sync(); spu_readch(MFC_RdAtomicStat); sync();
    sync();sync();sync();sync();sync();sync();
    spu_printf("ok\n");

    spu_printf("\n {0x%x} \n", spu_readch(SPU_RdEventStat));


    sys_spu_thread_exit(0);
	return 0;
}