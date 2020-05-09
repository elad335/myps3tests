#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>

#include "../spu_header.h"

static lock_line_t rdata;

inline void ack_event(u32 bit)
{
	// Ack events and set event mask
	spu_writech(SPU_WrEventMask, bit);
	spu_writech(SPU_WrEventAck, bit);

	if (spu_readchcnt(SPU_RdEventStat))
	{
		spu_readch(SPU_RdEventStat);
	}

	fsync();
}

int main(u64 raddr, u64 number)
{
	if (number == 1)
	{
		goto SKIP_EVENTS_TESTS;
	}

	// Test GETLLAR lr event raising (while raddr is different than EAL)

	ack_event(MFC_LLR_LOST_EVENT);

	mfc_getllar(rdata.data(), raddr + 128, 0, 0);
	spu_readch(MFC_RdAtomicStat);
	fsync();

	mfc_getllar(rdata.data(), raddr + 256, 0, 0);
	spu_readch(MFC_RdAtomicStat);
	fsync();

	spu_printf("events after GETLLAR: 0x%08x\n", spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);

	// Test PUTLLC lr event raising (while raddr is different than EAL)

	ack_event(MFC_LLR_LOST_EVENT);

	mfc_getllar(rdata.data(), raddr + 128, 0, 0);
	spu_readch(MFC_RdAtomicStat);
	fsync();

	mfc_putllc(rdata.data(), raddr + 256, 0, 0);
	spu_readch(MFC_RdAtomicStat);
	fsync();

	spu_printf("events after PUTLLC: 0x%08x\n", spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);

	// Test PUTLLUC lr event raising (while raddr is the same as EAL)

	ack_event(MFC_LLR_LOST_EVENT);

	mfc_getllar(rdata.data(), raddr + 128, 0, 0);
	spu_readch(MFC_RdAtomicStat);
	fsync();

	mfc_putlluc(rdata.data(), raddr + 128, 0, 0);
	spu_readch(MFC_RdAtomicStat);
	fsync();

	spu_printf("events after PUTLLUC: 0x%08x\n", spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);

	// Test PUT cmd lr event behaviour (while raddr is the same as EAL)

	ack_event(MFC_LLR_LOST_EVENT);

	mfc_getllar(rdata.data(), raddr + 128, 0, 0);
	spu_readch(MFC_RdAtomicStat);
	fsync();

	mfc_put(rdata.data(), raddr + 128, 0x80, 0, 0, 0);
	mfcsync();

	mfc_putllc(rdata.data(), raddr + 128, 0, 0);
	spu_printf("status after PUT:%d, events=0x%08x\n", spu_readch(MFC_RdAtomicStat), spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);

	SKIP_EVENTS_TESTS:
	mfcsync();
	ack_event(MFC_LLR_LOST_EVENT);

	u32 status;

	// Thread number 1 modifies reservation, Thread 0 waits for change 
	if (number == 1)
	{
		do
		{
			do {
			mfc_getllar(rdata.data(), raddr, 0, 0);
			spu_readch(MFC_RdAtomicStat);
			fsync();

			if (rdata.as<u32>(31) == -1u)
			{
				break;
			}

			rdata.as<u32>(0)++;
			fsync();

			mfc_putllc(rdata.data(), raddr, 0, 0);
			status = spu_readch(MFC_RdAtomicStat);
			fsync();
			} while (status);
		}
		while (rdata.as<u32>(31) != -1u);
	}
	else
	{
		u32 value;
		mfc_getllar(rdata.data(), raddr, 0, 0);
		spu_readch(MFC_RdAtomicStat);
		fsync();

		value = rdata.as<u32>(0);

		while (true)
		{
			mfc_getllar(rdata.data(), raddr, 0, 0);
			spu_readch(MFC_RdAtomicStat);
			fsync();

			if (value != rdata.as<u32>(0))
			{
				// Lose previous reservation
				mfc_getllar(rdata.data(), raddr + 128, 0, 0);
				spu_readch(MFC_RdAtomicStat);
				fsync();

				// Reservation changed earlier, event must be recorded
				spu_printf("events after GETLLAR loop: 0x%08x\n", spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);
				break;
			}
		}

		do {
		mfc_getllar(rdata.data(), raddr, 0, 0);
		spu_readch(MFC_RdAtomicStat);
		fsync();

		// Signal second thread to exit
		rdata.as<u32>(31) = -1u;
		fsync();

		mfc_putllc(rdata.data(), raddr, 0, 0);
		status = spu_readch(MFC_RdAtomicStat);
		fsync();
		} while (status);
	}

	sys_spu_thread_exit(0);
	return 0;
}