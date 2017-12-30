#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>

extern vec_int4 test(vec_int4 r3, void *scratch, void *failures, vec_int4 r6);

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;
	(void)arg4;

    char scratchBuf[8192] __attribute__((aligned(128))) = {0};
    char failedBuf[65536] = {0};

    spu_printf("Starting and running tests\n");

    vec_int4 res = test((vec_int4){0,0,0,0}, &scratchBuf, &failedBuf, (vec_int4){0,1,2,4});

    int numFailed = spu_extract(res, 0);
    spu_printf("done!\n");
    if (numFailed == 0) {
        spu_printf("No failed instructions detected\n");
    }
    else if (numFailed < 0) {
        spu_printf("test failed to bootstrap itself.\n");
    }
    else {
        /* 
        -- excerpt from cell-spu.s --

        # On return, if R3[0] > 0 then the buffer in R5 contains R3 failure records.
        # Each failure record is 16 words (64 bytes) long and has the following
        # format:
        #    Quadword 0, word 0: Failing instruction word
        #    Quadword 0, word 1: Address of failing instruction word
        #    Quadword 1: Instruction output value (where applicable)
        #    Quadword 2: Expected output value (where applicable)
        #    Quadword 3: FPSCR (where applicable) or other relevant data
        # Instructions have been coded so that the operands uniquely identify the
        # failing instruction; the address is provided as an additional aid in
        # locating the instruction.*/

        spu_printf("%d failed instructions.\n", numFailed);

        vec_int4 *fail = (vec_int4 *)&failedBuf;
        for (int i =0; i < numFailed; ++i) {
            spu_printf("----------------------------------------------------\n");
            spu_printf("Failed Inst word: 0x%x. Addr: 0x%x\n", spu_extract(*fail, 0), spu_extract(*fail, 1));
            fail += 1;
            spu_printf("Output: 0x%x 0x%x 0x%x 0x%x\n", spu_extract(*fail, 0), spu_extract(*fail, 1), spu_extract(*fail, 2), spu_extract(*fail, 3));
            fail += 1;
            spu_printf("Expected: 0x%x 0x%x 0x%x 0x%x\n", spu_extract(*fail, 0), spu_extract(*fail, 1), spu_extract(*fail, 2), spu_extract(*fail, 3));
            fail += 1;
            spu_printf("FPSCR: 0x%x 0x%x 0x%x 0x%x\n", spu_extract(*fail, 0), spu_extract(*fail, 1), spu_extract(*fail, 2), spu_extract(*fail, 3));
            fail += 1;
        }

        spu_printf("Throwing assert\n");
        __asm__ volatile ("stopd $2, $2, $2");
    }
    sys_spu_thread_exit(0);
	return 0;
}