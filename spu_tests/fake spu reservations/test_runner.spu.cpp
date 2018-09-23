#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

typedef int32_t s32;

#define int_cast(x) reinterpret_cast<uptr>(x)
#define ptr_cast(x) reinterpret_cast<void*>(x)

inline void _mm_mfence() { asm volatile ("syncc;sync;dsync"); }
inline void yield() { asm volatile ("stop 0x100"); }

// Sleep using yield implmentation, duration is in 1/80 the microsecond
inline void sleep_for(const u32 duration)
{
    spu_writech(SPU_WrDec, duration);
    while (s32(spu_readch(SPU_RdDec)) > 0)
    {
        yield();
    }
}

enum
{
    second = 80 * 1000000, 
};


static u8 arr[256] __attribute__((aligned(256))) = {0};

int main(uint64_t addr, uint64_t, uint64_t, uint64_t)
{
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

    sleep_for(10 * second);

    spu_printf("spu events: 0x%x\n", spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);

    spu_printf("sample finished");
    sys_spu_thread_exit(0);
	return 0;
}