#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

typedef uint64_t u64;
typedef uint32_t u32;

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    if (spu_readchcnt(SPU_RdInMbox))
    {
        spu_printf("spu_inbound_mailbox_ch value: 0x%x", spu_readch(SPU_RdInMbox));
    }
    asm volatile("lnop;sync;syncc;dsync");
    sys_spu_thread_exit(0);
	return 0;
}