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

	mfence();
}

// Atomic Status Update
enum
{
	MFC_PUTLLC_SUCCESS = 0,
	MFC_PUTLLC_FAILURE = 1, // reservation was lost
	MFC_PUTLLUC_SUCCESS = 2,
	MFC_GETLLAR_SUCCESS = 4,
};

int main(u64 addr1, u64 addr2)
{
	mfc_getllar(rdata.data(), addr1, 0, 0);
	spu_readch(MFC_RdAtomicStat);
	mfence();

	mfc_putllc(rdata.data(), addr2, 0, 0);
	const bool success = spu_readch(MFC_RdAtomicStat) == MFC_PUTLLC_SUCCESS;
	mfence();

	spu_printf("SPU PUTLLC status: %s\n", (success) ? "success" : "failure");
	return 0;
}