
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define __CELL_ASSERT__
#include <assert.h>
#include <sys/syscall.h>
#include <sys/gpio.h>
#include <cell.h>

//extern int test(int zero, void *scratch, void *failures, double one);

int main(void)
{
    register uint64_t cell __asm__ ("3");
    char *failedBuf = malloc(65536);

    printf("Starting / Running gpio tests\n");

    uint64_t magic = 988; // cause 988 is actaully cex only, allows to see what happens if the sycall available only on one type of console 
    system_call_0(magic);
        printf("cell %d = 0x%x\n",magic, cell);
    magic = 990;

    while (magic < 1024)
    {
	    system_call_0(magic);
        printf("cell %d = 0x%x\n",magic, cell);
        ++magic;
    }
    free(failedBuf);
    return 0;
}
