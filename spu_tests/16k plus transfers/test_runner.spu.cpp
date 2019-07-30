#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>
#include <string.h>

#include "../spu_header.h"

static u8 mfcData[0x4080] __attribute__((aligned(256)));

int main(u64 addr, u64, u64, u64) {

    spu_writech(MFC_EAL, 0);
    spu_writech(MFC_EAH, 0);
    spu_writech(MFC_LSA , int_cast(&mfcData[0]));
    spu_writech(MFC_TagID, 0x3f); // Invalid TAG
    spu_writech(MFC_Size, 0xC000); // Invalid Size
    spu_writech(MFC_Cmd, 0x42); // GETF
    mfcsync();

    spu_printf("success!\n");
    sys_spu_thread_exit(0);
	return 0;
}