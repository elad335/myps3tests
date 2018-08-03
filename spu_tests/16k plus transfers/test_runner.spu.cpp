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
static u8 mfcData[0x4080] __attribute__((aligned(256)));

int main(u64 addr, u64, u64, u64) {

    spu_writech(MFC_EAL, 0);
    spu_writech(MFC_EAH, 0);
    spu_writech(MFC_LSA , int_cast(&mfcData[0]));
    spu_writech(MFC_TagID, 0x3f); // Invalid TAG
    spu_writech(MFC_Size, 0xC000); // Invalid Size
    spu_writech(MFC_Cmd, 0x42); // GETF
    mfc_fence();

    spu_printf("success!\n");
    sys_spu_thread_exit(0);
	return 0;
}