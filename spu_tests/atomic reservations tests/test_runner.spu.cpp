#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

#define int_cast(x) reinterpret_cast<uptr>(x)
#define ptr_cast(x) reinterpret_cast<void*>(x)

inline void _mm_mfence() { asm volatile ("syncc;sync;dsync"); };

static u8 arr[256] __attribute__((aligned(256))) = {0};

int main(uint64_t, uint64_t, uint64_t, uint64_t)
{
    while(spu_readchcnt(SPU_RdSigNotify1) == 0){}
    uint32_t addr = spu_readch(SPU_RdSigNotify1);
    _mm_mfence();

    // ** Test GETLLAR stalling ** //

    // Ack events and set LR event mask
    spu_writech(SPU_WrEventAck, -1);
    if (spu_readchcnt(SPU_RdEventStat)) spu_readch(SPU_RdEventStat);
    spu_writech(SPU_WrEventMask, 0x400);
    _mm_mfence();

    spu_writech(MFC_EAL, addr);
    spu_writech(MFC_LSA, int_cast(&arr[0]));
    spu_writech(MFC_Cmd, 0xD0); // GETLLAR
    spu_readch(MFC_RdAtomicStat);
    _mm_mfence();

    spu_writech(MFC_EAL, addr);
    spu_writech(MFC_LSA, int_cast(&arr[0]));
    spu_writech(MFC_Cmd, 0xD0); // GETLLAR
    spu_readch(MFC_RdAtomicStat);
    _mm_mfence();

    spu_printf("didn't stall!; spu events: 0x%x\n", spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);

    // ** Test GETLLAR lr event raising (while raddr is different than EAL) ** //

    spu_writech(SPU_WrEventAck, -1);
    if (spu_readchcnt(SPU_RdEventStat)) spu_readch(SPU_RdEventStat);
    spu_writech(SPU_WrEventMask, 0x400);
    _mm_mfence();

    spu_writech(MFC_EAL, addr);
    spu_writech(MFC_LSA, int_cast(&arr[0]));
    spu_writech(MFC_Cmd, 0xD0); // GETLLAR
    spu_readch(MFC_RdAtomicStat);
    _mm_mfence();

    spu_writech(MFC_EAL, addr + 128);
    spu_writech(MFC_LSA, int_cast(&arr[0]));
    spu_writech(MFC_Cmd, 0xD0); // GETALLR
    spu_readch(MFC_RdAtomicStat);
    _mm_mfence();

    spu_printf("spu events after GETLLAR: 0x%x\n", spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);

    // ** Test PUTLLC lr event raising (while raddr is different than EAL) ** //

    spu_writech(SPU_WrEventAck, -1);
    if (spu_readchcnt(SPU_RdEventStat)) spu_readch(SPU_RdEventStat);
    spu_writech(SPU_WrEventMask, 0x400);
    _mm_mfence();

    spu_writech(MFC_EAL, addr);
    spu_writech(MFC_LSA, int_cast(&arr[0]));
    spu_writech(MFC_Cmd, 0xD0); // GETLLAR
    spu_readch(MFC_RdAtomicStat);
    _mm_mfence();

    spu_writech(MFC_EAL, addr + 128);
    spu_writech(MFC_LSA, int_cast(&arr[0]));
    spu_writech(MFC_Cmd, 0xB4); // PUTLLC
    spu_readch(MFC_RdAtomicStat);
    _mm_mfence();

    spu_printf("spu events after PUTLLC: 0x%x\n", spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);

    // ** Test PUTLLUC lr event raising (while raddr is the same as EAL) ** //

    spu_writech(SPU_WrEventAck, -1);
    if (spu_readchcnt(SPU_RdEventStat)) spu_readch(SPU_RdEventStat);
    spu_writech(SPU_WrEventMask, 0x400);
    _mm_mfence();

    spu_writech(MFC_EAL, addr);
    spu_writech(MFC_LSA, int_cast(&arr[0]));
    spu_writech(MFC_Cmd, 0xD0); // GETLLAR
    spu_readch(MFC_RdAtomicStat);
    _mm_mfence();

    spu_writech(MFC_EAL, addr);
    spu_writech(MFC_LSA, int_cast(&arr[0]));
    spu_writech(MFC_Cmd, 0xB0); // PUTLLUC
    spu_readch(MFC_RdAtomicStat);
    _mm_mfence();

    spu_printf("spu events after PUTLLUC: 0x%x\n", spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);

    spu_printf("sample finished");
    sys_spu_thread_exit(0);
	return 0;
}