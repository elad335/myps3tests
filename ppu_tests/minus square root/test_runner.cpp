
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define __CELL_ASSERT__
#include <assert.h>
#include <sys/syscall.h>
#include <sys/gpio.h>
#include <cell.h>
#include <ppu_asm_intrinsics.h>
#include <sstream>
#include <string>

//extern int test(int zero, void *scratch, void *failures, double one);
// Get contents of the CR register
uint32_t getCR()
{
    uint32_t CR;
    __asm__ ("mfcr %0" : "=r"(CR));
    return CR;
};

// Get contents of the FPSCR register
uint32_t getFPSCR()
{
    double FPSCR = 0.0;
    __asm__ ("mffs %0" : "=f"(FPSCR));
    return ((uint32_t*)&FPSCR)[1];
};

// Clear CR register
void clearCR()
{
    uint32_t CR = 0;
    __asm__ ("mtcr %0" : "=r"(CR));
};

// Clear FPSCR register
void clearFPSCR()
{
    __asm__ ("mtfsfi 0, 0");
    __asm__ ("mtfsfi 1, 0");
    __asm__ ("mtfsfi 2, 0");
    __asm__ ("mtfsfi 3, 0");
    __asm__ ("mtfsfi 4, 0");
    __asm__ ("mtfsfi 5, 0");
    __asm__ ("mtfsfi 6, 0");
    __asm__ ("mtfsfi 7, 0");
};

int main(void)
{
   void *failedBuf = malloc(65536);
double ugh = -1.0;

    printf("%llf\n",__fsqrt( ugh ));
    printf("exception handler didnt invoke");
    free(failedBuf);
    return 0;
}
