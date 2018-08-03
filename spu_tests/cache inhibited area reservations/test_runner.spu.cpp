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
static u64 rdata[16] __attribute__((aligned(256))) = {};

int main(u64 raddr, u64, u64, u64) {

    spu_writech(MFC_EAL, raddr);
    spu_writech(MFC_EAH, 0);
    spu_writech(MFC_LSA , int_cast(&rdata[0]));
    spu_writech(MFC_Cmd, 0xD0); // GETLLAR
    mfence(); spu_readch(MFC_RdAtomicStat); mfence();

    spu_printf("rdata:\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    "%llx\n"
    , ptr_dref(rdata + 0, u64)
    , ptr_dref(rdata + 1, u64)
    , ptr_dref(rdata + 2, u64)
    , ptr_dref(rdata + 3, u64)
    , ptr_dref(rdata + 4, u64)
    , ptr_dref(rdata + 5, u64)
    , ptr_dref(rdata + 6, u64)
    , ptr_dref(rdata + 7, u64)
    , ptr_dref(rdata + 8, u64)
    , ptr_dref(rdata + 9, u64)
    , ptr_dref(rdata + 10, u64)
    , ptr_dref(rdata + 11, u64)
    , ptr_dref(rdata + 12, u64)
    , ptr_dref(rdata + 13, u64)
    , ptr_dref(rdata + 14, u64)
    , ptr_dref(rdata + 15, u64));
    mfence();

    spu_writech(MFC_EAL, raddr); // aligned
    spu_writech(MFC_LSA , int_cast(&rdata[0])); // aligned
    spu_writech(MFC_Cmd, 0xB4); // PUTLLC
    mfence(); spu_printf("reservation status: 0x%x", spu_readch(MFC_RdAtomicStat)); mfence();

    sys_spu_thread_exit(0);
	return 0;
}