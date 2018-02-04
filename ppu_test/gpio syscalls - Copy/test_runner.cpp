
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define __CELL_ASSERT__
#include <assert.h>
#include <sys/syscall.h>
#include <sys/gpio.h>
#include <cell.h>
#include <string>

int main(void)
{
    register uint64_t cell __asm__ ("3");

    printf("Running tests\n");

    uint64_t magic = 60; // sys_trace 60 - 69
    FILE * pFile = fopen ("/dev_hdd0/output.txt","w");

    while (magic < 70)
    {
        system_call_0(magic);
        uint64_t tempcall = cell;
        fprintf(pFile, "syscall %d returned 0x%x",magic, tempcall);
        ++magic;
    }
    
    fclose(pFile);

    return 0;
}
