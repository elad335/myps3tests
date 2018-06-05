#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>

#define u32 uint32_t
#define u64 uint64_t
#define uptr uintptr_t

#define sync asm volatile ("lnop;syncc;sync;dsync;syncc;sync;dsync;syncc;sync;dsync")

int main(void) {

    while(spu_readchcnt(SPU_RdSigNotify1) == 0){}
    spu_printf("ok\n");
    char arr[8192] __attribute__((aligned(256))) = {0};
    char* ptr = &arr[0];
    uint32_t addr = spu_readch(SPU_RdSigNotify1);
    spu_printf("ok\n");
    sync;

    spu_writech(SPU_WrEventMask, 0x400); // LR event
    sync;
    if (spu_readchcnt(SPU_RdEventStat)) 
    {
        spu_writech(SPU_WrEventAck, spu_readch(SPU_RdEventStat)); // if count is 1, make it 0 and ack existing events
    }
    sync;
    spu_printf("ok\n");

    spu_writech(MFC_TagID, 0);

    for (u32 i = 0; i < 0x10; i++)
    {
        spu_writech(MFC_EAL, 0); // Different address than the current reservation
        spu_writech(MFC_LSA , (uptr)ptr);
        spu_writech(MFC_Cmd, 0xB4); // GETLLAR
        sync; spu_printf("getllar: 0x%x \n", spu_readch(MFC_RdAtomicStat)); sync;
    }

    spu_printf("\n {0x%x} \n", spu_readchcnt(SPU_RdEventStat));


    sys_spu_thread_exit(0);
	return 0;
}