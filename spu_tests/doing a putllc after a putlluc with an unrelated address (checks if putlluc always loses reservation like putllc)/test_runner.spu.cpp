#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

// Atomic Status Update
enum
{
	MFC_PUTLLC_SUCCESS = 0,
	MFC_PUTLLC_FAILURE = 1, // reservation was lost
	MFC_PUTLLUC_SUCCESS = 2,
	MFC_GETLLAR_SUCCESS = 4,
};

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
    spu_writech(MFC_Cmd, 0xD0); // GETLLAR
    sync(); spu_readch(MFC_RdAtomicStat); sync();
    sync();sync();sync();sync();sync();sync();
    spu_printf("ok\n");

    spu_writech(MFC_EAL, addr/* + 0x100*/); // Different address than the current reservation
    spu_writech(MFC_LSA , (uptr)ptr + 0x100);
    spu_writech(MFC_Cmd, 0xB0); // PUTLLC
    sync(); spu_readch(MFC_RdAtomicStat); sync();
    sync();sync();sync();sync();sync();sync();
    spu_printf("ok\n");

    spu_writech(MFC_EAL, addr); // Different address than the current reservation
    spu_writech(MFC_LSA , (uptr)ptr);
    spu_writech(MFC_Cmd, 0xB4); // PUTLLC
    sync(),sync(),sync(),sync(),sync(),sync();

    if (spu_readch(MFC_RdAtomicStat) == MFC_PUTLLC_SUCCESS)
    {
        spu_printf("PUTLLC succeeded:)");
    }
    else {
        spu_printf("PUTLLC failed.");
    }

    spu_printf("spu_event_ch count: 0x%x", spu_readchcnt(SPU_RdEventStat));

    sys_spu_thread_exit(0);
	return 0;
}