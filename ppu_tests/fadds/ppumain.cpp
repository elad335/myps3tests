
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

#include "ppu_asm_intrinsics.h"
#include "../ppu_header.h"

static __attribute__((noinline)) u64 test_fps(double a, double b)
{
    const double res = __fadds(a, b);
    return *reinterpret_cast<const u64*>(&res);
}

static __attribute__((noinline)) u32 _test_fps(double a, double b)
{
    const float res = (float)__fadds(a, b);
    return *reinterpret_cast<const u32*>(&res);
}

static __attribute__((noinline)) u64 test_fps_(double a, double b)
{
    const double res = __fadd(a, b);
    return *reinterpret_cast<const u64*>(&res);
}

static __attribute__((noinline)) u32 _test_fps_(double a, double b)
{
    const float res = (float)__fadd(a, b);
    return *reinterpret_cast<const u32*>(&res);
}
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
    static volatile double a = 1.000045; 
    static volatile double b = 1.000050;
    volatile float a_ = (float)a;
    volatile double bits_a = (double)a_;
    if (*reinterpret_cast<volatile u64*>(&bits_a) == *reinterpret_cast<volatile u64*>(&a)) printf("Ugh!\n");

    //printf("before:%llx\n", *reinterpret_cast<volatile u64*>(&a));
    //printf("before:%llx, (f32)=%x\n", *reinterpret_cast<volatile u64*>(&bits_a), *reinterpret_cast<volatile u32*>(&a_));
    //printf("%llx\n", test_fps(a, 0));
    //printf("%llx\n", test_fps(a_, 0));
    printf("fadds:\n");
    printf("a+b=%llx\n", test_fps(a, b));
    volatile float a__ = a;
    volatile float b__ = b;
    printf("f32(a)+f32(b)=%llx\n", test_fps(a__, b__));
    printf("a+b=(f32)%llx\n", _test_fps(a, b));
    printf("f32(a)+f32(b)=(f32)%llx\n", _test_fps(a__, b__));
     printf("fadd:\n");
    printf("a+b=%llx\n", test_fps_(a, b));
    printf("f32(a)+f32(b)=%llx\n", test_fps_(a__, b__));
    printf("a+b=(f32)%llx\n", _test_fps_(a, b));
    printf("f32(a)+f32(b)=(f32)%llx\n", _test_fps_(a__, b__));
    //printf("exception handler didnt invoke\n");
    return 0;
}
