#include <sys/spu_thread.h>
#include <sys/event_flag.h>
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

typedef uint32_t u32;
static u32 ret = 0, i = 0;
register u32 stack asm("1");
register u32 save_stack1 asm("124");
register u32 save_stack2 asm("123");  
static u32 mails[4] = {};

inline void empty_mailbox()
{
    i = spu_readchcnt(SPU_RdInMbox);
    for (u32 res = 0; res < i; ++res)
    {
        mails[res] = spu_readch(SPU_RdInMbox);
    }
    spu_printf("malibox mails amount: 0x%x, ret=0x%x", i, ret);

    for (u32 res = 0; res < i; ++res)
    {
        spu_printf(" 0x%x ", mails[res]);
    }

    spu_printf("\n");
}

int main()
{

    //empty_mailbox();

    i = spu_readchcnt(SPU_RdInMbox);
    if (spu_readchcnt(SPU_RdInMbox))
    for (;spu_readchcnt(SPU_RdInMbox);)
    {
        spu_readch(SPU_RdInMbox);
    }

    save_stack1 = stack;
    //spu_writech(SPU_WrOutMbox, spu_readch(SPU_RdSigNotify1));
    //spu_writech(SPU_WrOutIntrMbox, (192u << 24));
    sys_event_flag_set_bit(spu_readch(SPU_RdSigNotify1), 0);
    save_stack2 = stack;

    i = spu_readchcnt(SPU_RdInMbox);
    for (u32 res = 0; res < i; ++res)
    {
        mails[res] = spu_readch(SPU_RdInMbox);
    }
    //while(1);

    spu_printf("before: 0x%x after: 0x%x\n", save_stack1, save_stack2);
    si_stop(0x3fff);

    sys_spu_thread_exit(0);
    return 0;
}