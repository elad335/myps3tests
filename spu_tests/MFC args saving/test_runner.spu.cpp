#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>
#include <string.h>

typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t uptr;
typedef uint8_t u8;

#define ptr_cast(x, T) reinterpret_cast<T*>(x)
#define ptr_dref(x, T) *reinterpret_cast<T*>(x)
#define int_cast(x) reinterpret_cast<u32>(x)

inline void mfence() { asm volatile ("syncc;sync;dsync"); };
inline void mfc_fence() 
{ 
    spu_writech(MFC_WrTagMask, -1u);spu_writech(MFC_WrTagUpdate, 2); 
    si_rdch(MFC_RdTagStat); mfence();
};
static u8 mfcData[256] __attribute__((aligned(256))) = {};

int main(u64 addr, u64, u64, u64) {

    spu_writech(MFC_EAL, addr);
    spu_writech(MFC_EAH, 0);
    spu_writech(MFC_LSA , int_cast(&mfcData[0]));
    spu_writech(MFC_Size, 0x10);
    spu_writech(MFC_Cmd, 0x42); // GETF
    mfc_fence();
    memset(mfcData, 0, 0x10);
    spu_writech(MFC_Cmd, 0x42); // GETF
    mfc_fence();

    spu_printf("data:\n"
    "%x\n"
    "%x\n"
    "%x\n"
    "%x\n"
    , ptr_dref(mfcData + 0, u32)
    , ptr_dref(mfcData + 4, u32)
    , ptr_dref(mfcData + 8, u32)
    , ptr_dref(mfcData + 12, u32));

    sys_spu_thread_exit(0);
	return 0;
}