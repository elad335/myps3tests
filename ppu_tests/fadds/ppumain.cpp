
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

#define static_noinline static __attribute__((noinline))

// Fadd/s
static_noinline u64 fadds_f64(double a, double b)
{
    const double res = __fadds(a, b);
    return *reinterpret_cast<const u64*>(&res);
}

static_noinline u32  fadds_f32(double a, double b)
{
    const float res = (float)__fadds(a, b);
    return *reinterpret_cast<const u32*>(&res);
}

static_noinline u64 fadd_f64(double a, double b)
{
    const double res = __fadd(a, b);
    return *reinterpret_cast<const u64*>(&res);
}

static_noinline u32 fadd_f32(double a, double b)
{
    const float res = (float)__fadd(a, b);
    return *reinterpret_cast<const u32*>(&res);
}


// Fmul/s
static_noinline u64 fmuls_f64(double a, double b)
{
    const double res = __fmuls(a, b);
    return *reinterpret_cast<const u64*>(&res);
}

static_noinline u32 fmuls_f32(double a, double b)
{
    const float res = (float)__fmuls(a, b);
    return *reinterpret_cast<const u32*>(&res);
}

static_noinline u64 fmul_f64(double a, double b)
{
    const double res = __fmul(a, b);
    return *reinterpret_cast<const u64*>(&res);
}

static_noinline u32 fmul_f32(double a, double b)
{
    const float res = (float)__fmul(a, b);
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
    // Adding contands
    static volatile double a = 1.00000017; 
    static volatile double b = 1.00000017;
    volatile float a_f32 = a;
    volatile float b_f32 = b;

    // Multiplying constants
    static volatile double c = 1.00000017; 
    static volatile double d = 1000000.0;
    volatile float c_f32 = a;
    volatile float d_f32 = b;

    // Floats as f64
    volatile double bits_a = (double)a_f32;
    volatile double bits_b = (double)b_f32;
    volatile double bits_c = (double)c_f32;
    volatile double bits_d = (double)d_f32;

    if (*reinterpret_cast<volatile u64*>(&bits_a) == *reinterpret_cast<volatile u64*>(&a)
    || *reinterpret_cast<volatile u64*>(&bits_b) == *reinterpret_cast<volatile u64*>(&b)
    || *reinterpret_cast<volatile u64*>(&bits_c) == *reinterpret_cast<volatile u64*>(&c))
    //|| *reinterpret_cast<volatile u64*>(&bits_d) == *reinterpret_cast<volatile u64*>(&d))
    {
        // A number from our selection is perfectly represtable as float and double 
        printf("Ugh!\n");
    }

    //printf("before:%llx\n", *reinterpret_cast<volatile u64*>(&a));
    //printf("before:%llx, (f32)=%x\n", *reinterpret_cast<volatile u64*>(&bits_a), *reinterpret_cast<volatile u32*>(&a_));
    //printf("%llx\n", fadds_f64(a, 0));
    //printf("%llx\n", fadds_f64(a_, 0));

    // Adding
    /*
    printf("fadds:\n");
    printf("a+b=%llx\n", fadds_f64(a, b));

    printf("f32(a)+f32(b)=%llx\n", fadds_f64(a_f32, b_f32));
    printf("a+b=(f32)%llx\n",  fadds_f32(a, b));
    printf("f32(a)+f32(b)=(f32)%llx\n", fadds_f32(a_f32, b_f32));
    printf("fadd:\n");
    printf("a+b=%llx\n", fadd_f64(a, b));
    printf("f32(a)+f32(b)=%llx\n", fadd_f64(a_f32, b_f32));
    printf("a+b=(f32)%llx\n", fadd_f32(a, b));
    printf("f32(a)+f32(b)=(f32)%llx\n", fadd_f32(a_f32, b_f32));
    */

    // Mulptiplying
    printf("fmuls:\n");
    printf("a*b=%llx\n", fmuls_f64(a, b));

    printf("f32(a)*f32(b)=%llx\n", fmuls_f64(a_f32, b_f32));
    printf("a*b=(f32)%llx\n", fmuls_f32(a, b));
    printf("f32(a)*f32(b)=(f32)%llx\n",  fmuls_f32(a_f32, b_f32));
    printf("fmul:\n");
    printf("a*b=%llx\n", fmul_f64(a, b));
    printf("f32(a)*f32(b)=%llx\n", fmul_f64(a_f32, b_f32));
    printf("a*b=(f32)%llx\n", fmul_f32(a, b));
    printf("f32(a)*f32(b)=(f32)%llx\n", fmul_f32(a_f32, b_f32));

    //printf("exception handler didnt invoke\n");
    return 0;
}
