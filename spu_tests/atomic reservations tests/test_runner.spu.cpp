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

int main(u64 raddr)
{
    // Test GETLLAR lr event raising (while raddr is different than EAL)

    ack_event(MFC_LLR_LOST_EVENT);

    mfc_getllar(rdata.data(), raddr, 0, 0);
    spu_readch(MFC_RdAtomicStat);
    mfence();

    mfc_getllar(rdata.data(), raddr + 128, 0, 0);
    spu_readch(MFC_RdAtomicStat);
    mfence();

    spu_printf("events after GETLLAR: 0x%08x\n", spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);

    // Test PUTLLC lr event raising (while raddr is different than EAL)

    ack_event(MFC_LLR_LOST_EVENT);

    mfc_getllar(rdata.data(), raddr, 0, 0);
    spu_readch(MFC_RdAtomicStat);
    mfence();

    mfc_putllc(rdata.data(), raddr + 128, 0, 0);
    spu_readch(MFC_RdAtomicStat);
    mfence();

    spu_printf("events after PUTLLC: 0x%08x\n", spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);

    // Test PUTLLUC lr event raising (while raddr is the same as EAL)

    ack_event(MFC_LLR_LOST_EVENT);

    mfc_getllar(rdata.data(), raddr, 0, 0);
    spu_readch(MFC_RdAtomicStat);
    mfence();

    mfc_putlluc(rdata.data(), raddr, 0, 0);
    spu_readch(MFC_RdAtomicStat);
    mfence();

    spu_printf("events after PUTLLUC: 0x%08x\n", spu_readchcnt(SPU_RdEventStat) ? spu_readch(SPU_RdEventStat) : 0);

    spu_printf("sample finished");
    sys_spu_thread_group_exit(0);
	return 0;
}