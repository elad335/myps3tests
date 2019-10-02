#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <spu_mfcio.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

inline void sync() {asm volatile ("sync;syncc;dsync");}

// Gets the address of LS in an spu thread in the group
#define ls_to_mmio(spu_num, offs) ((spu_num * SYS_SPU_THREAD_OFFSET) + offs + SYS_SPU_THREAD_LS_BASE + SYS_SPU_THREAD_BASE_LOW)

static u8 DmaBuffer[0x10] __attribute__((aligned(128)));

// Receive the targeted thread number from the PPU
int main(uint64_t spu_num, uint64_t, uint64_t, uint64_t)
{
	spu_mfcdma32(&DmaBuffer[0], ls_to_mmio(spu_num, 0), 0x10, 0, MFC_PUTB_CMD); // Send data in MMIO to the thread LS itself
	spu_mfcstat(MFC_TAG_UPDATE_ALL);
	sync();
	spu_printf("sample finished successfully\n");
	return 0;
}