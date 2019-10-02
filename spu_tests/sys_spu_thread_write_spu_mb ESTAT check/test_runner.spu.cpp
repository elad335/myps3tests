#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <spu_mfcio.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#include "../spu_header.h"

static u32 count __attribute__((aligned(16)));

int main(uint64_t addr, uint64_t, uint64_t, uint64_t)
{
	count = spu_readchcnt(SPU_RdInMbox);
	spu_mfcdma32(&count, addr, sizeof(u32), 0, MFC_PUTB_CMD);
	spu_mfcstat(MFC_TAG_UPDATE_ALL);
	mfence();
	sys_spu_thread_group_exit(0);
	return 0;
}