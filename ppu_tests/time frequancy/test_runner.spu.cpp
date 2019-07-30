#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

#include "../../spu_tests/spu_header.h"

static volatile s32 _signal = 1;
static volatile u32 time_array[4] __attribute__((aligned(16)));

int main(void)
{
	u32 spu_ptr = spu_readch(SPU_RdInMbox);

	// Workaround for rpcs3: wait for ppu to finnish mmio
	{
		lock_line_t dummy;
		for (u32 i = 0; i < 10; i++)
		{
			mfc_getllar(&dummy, spu_ptr, 0, 0);
			mfc_putllc(&dummy, spu_ptr, 0, 0);
		}
	}

	u32 before, after, passed;

	spu_writech(SPU_WrDec, (u32)-1); // Reset Timer

	mfc_put(&_signal, spu_ptr + sizeof(u32) * 3, sizeof(u32), 0, 0, 0); // Signal running
	mfcsync();

	before = spu_readch(SPU_RdDec);

	do
	{
		mfc_getb(&_signal, spu_ptr + sizeof(u32) * 3, sizeof(u32), 0, 0, 0); // Signal running;
	}
	while (_signal != -1);

	after = spu_readch(SPU_RdDec);

	// This is a decrementer, time is counting down
	passed = before - after;

	time_array[0] = before;
	time_array[1] = after;
	time_array[2] = passed;

	asm volatile ("dsync");

	mfc_put(&time_array[0], spu_ptr, sizeof(u32) * 4, 0, 0, 0); // Send data
	mfcsync();
	return 0;
}
