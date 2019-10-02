#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>
#include <string.h>

#include "../spu_header.h"

static mfc_list_element_t mfcData[16] __attribute__((aligned(128)));

int main(u64 addr) {

	mfcsync();

	// The first elements fetches 128 bytes on list
	mfcData[0].size = 0x80;
	mfcData[0].eal = addr;
	mfcData[0].notify = 0;

	// Fill the rest of the elements with NOP transfers
	for (u32 i = 1; i < 16; i++)
	{
		mfcData[i].size = 0;
		mfcData[i].notify = 0;
		mfcData[i].eal = addr;
	}

	mfc_getl(&mfcData[0], 0, &mfcData[0], 0x30, 0, 0,0);
	mfcsync();
	sys_spu_thread_exit(0);
	return 0;
}