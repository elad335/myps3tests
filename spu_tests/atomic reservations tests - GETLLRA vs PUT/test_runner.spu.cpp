#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>
#include <string.h>

#include "../spu_header.h"

static lock_line_t rdata0, rdata1, rdata;

inline bool btr_events(u32 bit)
{
	// Ack events and set event mask
	const u32 mask = spu_readch(SPU_RdEventMask);

	spu_writech(SPU_WrEventMask, bit);

	u32 ret = 0;
	if (spu_readchcnt(SPU_RdEventStat))
	{
		ret = spu_readch(SPU_RdEventStat);
	}

	spu_writech(SPU_WrEventAck, bit);
	spu_writech(SPU_WrEventMask, mask);

	if (spu_readchcnt(SPU_RdEventStat))
	{
		spu_readch(SPU_RdEventStat);
	}

	fsync();
	return !!(ret & bit);
}

int main(u64 raddr, u64 number)
{
	// Reset to 0
	std::memset(rdata0.data(), 0, 0x80);

	// Reset to 1
	std::memset(rdata1.data(), 0xff, 0x80);

	for (u64 i = 0; i < 100000000; i++)
	{
		const u32 offs = 0x0;
		const u32 size = 0x80;

		if (number)
		{
			mfc_putf(rdata1.data(), raddr + offs, size, 0, 0, 0);
			mfc_putf(rdata0.data(), raddr + offs, size, 0, 0, 0);
			continue;
		}

		mfc_getllar(rdata.data(), raddr, 0, 0);
		spu_readch(MFC_RdAtomicStat);
		fsync();

		bool invalid = true;
		switch (rdata.as<u8>(offs))
		{
		case 0xff:
		{
			// Compare with ones
			invalid = !!std::memcmp(rdata.data(offs), rdata1.data(), size);
			break;
		}
		case 0x00:
		{
			// Compare with zeroes
			invalid = !!std::memcmp(rdata.data(offs), rdata0.data(), size);
			break;
		}
		default: break; // invalid
		}

		if (invalid)
		{
			mfcsync();
			spu_printf("Non-atomic access detected! (i=0x%llx)\n", i);
			return 0;
		}
	}

	mfcsync();
	if (!number) spu_printf("All accesses were atomic!\n");
	return 0;
}