#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <spu_mfcio.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

typedef uint64_t u64;
typedef uint32_t u32;

#define int_cast(x) reinterpret_cast<uintptr_t>(x)
#define ptr_cast(x) reinterpret_cast<void*>(x)

inline void _mm_mfence() { asm volatile ("syncc;sync;dsync"); }
inline void yield() { asm volatile ("stop 0x100"); }

static u32 count __attribute__((aligned(16)));

int main(uint64_t addr, uint64_t, uint64_t, uint64_t)
{
    count = spu_readchcnt(SPU_RdInMbox);
    spu_mfcdma32(&count, addr, sizeof(u32), 0, MFC_PUTB_CMD);
    spu_mfcstat(MFC_TAG_UPDATE_ALL);
    asm volatile("lnop;sync;syncc;dsync");
    //sys_spu_thread_exit(0);
    sys_spu_thread_group_exit(0);
	return 0;
}