#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_mfcio_gcc.h>
#include <spu_internals.h>
#include <stdint.h>

#define u32 uint32_t
#define u64 uint64_t
#define uptr uintptr_t

#define int_cast(x) reinterpret_cast<uptr>(x)

inline void _mm_mfence() { asm volatile ("syncc;sync;dsync;syncc;sync;dsync"); };

int main(void) {

    uint32_t addr = spu_readch(SPU_RdSigNotify1); // Get the mfc address sent by the ppu thread
    u32 arr[1024] __attribute__((aligned(256))) = {0x00008000, addr};
    u32 arr2[1024] __attribute__((aligned(256))) = {0};
    _mm_mfence();

    spu_writech(MFC_EAL, int_cast(&arr[0]));
    spu_writech(MFC_LSA , int_cast(&arr2[0]));
    spu_writech(MFC_TagID , 0);
    spu_writech(MFC_Size , 8);
    spu_writech(MFC_Cmd, 0x44); // GETL 
    _mm_mfence();
    mfc_write_tag_mask(1); // The list's tag group
    _mm_mfence();
    mfc_write_tag_update_all(); // Wait for the list command to complete
    spu_printf("cell_ok!\nthe mfc didnt stall!");

    sys_spu_thread_exit(0);
	return 0;
}