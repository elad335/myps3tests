#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>
#include <string.h>

#include "../spu_header.h"

static u8 mfcData[256] __attribute__((aligned(256))) = {};

int main(u64 addr, u64, u64, u64) {

	u32 listCmds[4] __attribute__((aligned(256))) = {0x0, addr, 0x10, addr};
	spu_writech(MFC_EAL, int_cast(&listCmds[0]));
	spu_writech(MFC_EAH, 0);
	spu_writech(MFC_LSA , int_cast(&mfcData[0]));
	spu_writech(MFC_Size, 0x10);
	spu_writech(MFC_Cmd, 0x44); // GETLF
	mfcsync();

	spu_printf("data:\n"
	"%x\n"
	"%x\n"
	"%x\n"
	"%x\n"
	, *ptr_caste(mfcData + 0, u32)
	, *ptr_caste(mfcData + 4, u32)
	, *ptr_caste(mfcData + 8, u32)
	, *ptr_caste(mfcData + 12, u32));

	sys_spu_thread_exit(0);
	return 0;
}