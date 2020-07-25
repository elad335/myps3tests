/**
 * (c) 2015 Alexandro Sanchez Bach. All rights reserved.
 * Released under GPL v2 license. Read LICENSE for more details.
 */

#include <stdio.h>

//#include "../../../common/output.h"

// Show results
#define PRINT1(name, i, GPR) \
    printf(name"([%02d]) -> %016llX [%016llX : %08X]\n", i, +GPR, _xer, _cr);
#define PRINT2(name, i, j, GPR) \
    printf(name"([%02d],[%02d]) -> %016llX [%016llX : %08X]\n", i, j, +GPR, _xer, _cr);
#define PRINT3(name, i, j, k, GPR) \
    printf(name"([%02d],[%02d],[%02d]) -> %016llX [%016llX : %08X]\n", i, j, k, +GPR, _xer, _cr);
#define PRINT4(name, i, j, k, l, GPR) \
    printf(name"([%02d],[%02d],[%02d],[%02d]) -> %016llX [%016llX : %08X]\n", i, j, k, l, +GPR, _xer, _cr);

// Iterate
#define ITERATE1(x) \
    for (uint32_t i=0; i<sizeof(testInts64)/sizeof(testInts64[0]); i++) \
        {uint64_t r0 = -1, r1=testInts64[i], _xer = -1; uint32_t _cr; x;}

#define ITERATE2(x) \
    for (uint32_t i=0; i<sizeof(testInts64)/sizeof(testInts64[0]); i++) \
        for (uint32_t j=0; j<sizeof(testInts64)/sizeof(testInts64[0]); j++) \
            {uint64_t r0 = -1, r1=testInts64[i], r2=testInts64[j], _xer = -1; uint32_t _cr; x;}

// Get contents of the CR register
uint32_t getCR()
{
    uint32_t CR;
    __asm__ ("mfcr %0" : "=r"(CR));
    return CR;
};

// Get contents of the XER register
uint64_t getXER()
{
    uint64_t XER;
    __asm__ ("mfxer %0" : "=r"(XER));
    return XER;
};

// Clear CR register
void clearCR()
{
    uint32_t CR = 0;
    __asm__ ("mtcr %0" : "=r"(CR));
};

// Clear XER register
void clearXER()
{
    uint64_t XER = 0;
    __asm__ ("mtxer %0" : "=r"(XER));
};

#define clearCrXer ";mtxer %3; mtcr %3;"
#define getCrXer ";mfxer %1; mfcr %2;"
#define clobberCrXer "xer", "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7"

uint64_t testInts64[] = {
    0x0000000000000000LL, 0x0000000000000001LL, 0x0000000000000002LL, 0xFFFFFFFFFFFFFFFFLL,  //  0  1  2  3
    0xFFFFFFFFFFFFFFFELL, 0x0003333300330033LL, 0x000000FFFFF00000LL, 0x1000000000000000LL,  //  4  5  6  7
    0x1FFFFFFFFFFFFFFFLL, 0x4238572200000000LL, 0x7000000000000000LL, 0x0000000072233411LL,  //  8  9 10 11
    0x7FFFFFFFFFFFFFFFLL, 0x8000000000000000LL, 0x8000000000000001LL, 0x0123456789ABCDEFLL,  // 12 13 14 15
};

int main(void)
{

    uint32_t zero = 0;

    // Integer Arithmetic Instructions
    ITERATE2(__asm__ (clearCrXer "add     %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("add    ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "add.    %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("add.   ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "addc    %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("addc   ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "addc.   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("addc.  ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "adde    %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("adde   ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "adde.   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("adde.  ",i,j,r0));
    ITERATE1(__asm__ (clearCrXer "addi    %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(+0) : clobberCrXer);   PRINT2("addi   ",i,0,r0));
    ITERATE1(__asm__ (clearCrXer "addi    %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(+1) : clobberCrXer);   PRINT2("addi   ",i,1,r0));
    ITERATE1(__asm__ (clearCrXer "addi    %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);  PRINT2("addi   ",i,-1,r0));
    ITERATE1(__asm__ (clearCrXer "addic   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(+0) : clobberCrXer);   PRINT2("addic  ",i,0,r0));
    ITERATE1(__asm__ (clearCrXer "addic   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(+1) : clobberCrXer);   PRINT2("addic  ",i,1,r0));
    ITERATE1(__asm__ (clearCrXer "addic   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);  PRINT2("addic  ",i,-1,r0));
    ITERATE1(__asm__ (clearCrXer "addic.  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(+0) : clobberCrXer);   PRINT2("addic. ",i,0,r0));
    ITERATE1(__asm__ (clearCrXer "addic.  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(+1) : clobberCrXer);   PRINT2("addic. ",i,1,r0));
    ITERATE1(__asm__ (clearCrXer "addic.  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);  PRINT2("addic. ",i,-1,r0));
    ITERATE1(__asm__ (clearCrXer "addis   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(+0) : clobberCrXer);   PRINT2("addis  ",i,0,r0));
    ITERATE1(__asm__ (clearCrXer "addis   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(+1) : clobberCrXer);   PRINT2("addis  ",i,1,r0));
    ITERATE1(__asm__ (clearCrXer "addis   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);  PRINT2("addis  ",i,-1,r0));
    ITERATE1(__asm__ (clearCrXer "addme   %0,%4"    getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);   PRINT1("addme  ",i,r0));
    ITERATE1(__asm__ (clearCrXer "addme.  %0,%4"    getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);  PRINT1("addme. ",i,r0));
    ITERATE1(__asm__ (clearCrXer "addze   %0,%4"    getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);  PRINT1("addze  ",i,r0));
    ITERATE1(__asm__ (clearCrXer "addze.  %0,%4"    getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);  PRINT1("addze. ",i,r0));
    ITERATE2(__asm__ (clearCrXer "divd    %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("divd   ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "divd.   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("divd.  ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "divdu   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("divdu  ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "divdu.  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("divdu. ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "divw    %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("divw   ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "divw.   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("divw.  ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "divwu   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("divwu  ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "divwu.  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("divwu. ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "mulhd   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("mulhd  ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "mulhd.  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("mulhd. ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "mulhdu  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("mulhdu ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "mulhdu. %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("mulhdu.",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "mulhw   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("mulhw  ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "mulhw.  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("mulhw. ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "mulhwu  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("mulhwu ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "mulhwu. %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("mulhwu.",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "mulld   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("mulld  ",i,j,r0));
    ITERATE1(__asm__ (clearCrXer "mulli   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(+0) : clobberCrXer);   PRINT2("mulli  ",i,0,r0));
    ITERATE1(__asm__ (clearCrXer "mulli   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(+1) : clobberCrXer);   PRINT2("mulli  ",i,1,r0));
    ITERATE1(__asm__ (clearCrXer "mulli   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);  PRINT2("mulli  ",i,-1,r0));
    ITERATE2(__asm__ (clearCrXer "mullw   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("mullw  ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "mullw.  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("mullw. ",i,j,r0));
    ITERATE1(__asm__ (clearCrXer "neg     %0,%4"    getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);      PRINT1("neg    ",i,r0));
    ITERATE1(__asm__ (clearCrXer "neg.    %0,%4"    getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);      PRINT1("neg.   ",i,r0));
    ITERATE2(__asm__ (clearCrXer "subf    %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("subf   ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "subf.   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("subf.  ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "subfc   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("subfc  ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "subfc.  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("subfc. ",i,j,r0));
    ITERATE1(__asm__ (clearCrXer "subfic  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(+0) : clobberCrXer);   PRINT2("subfic ",i,0,r0));
    ITERATE1(__asm__ (clearCrXer "subfic  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(+1) : clobberCrXer);   PRINT2("subfic ",i,1,r0));
    ITERATE1(__asm__ (clearCrXer "subfic  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);  PRINT2("subfic ",i,-1,r0));
    ITERATE2(__asm__ (clearCrXer "subfe   %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("subfe  ",i,j,r0));
    ITERATE2(__asm__ (clearCrXer "subfe.  %0,%4,%5" getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "r"(r2) : clobberCrXer);  PRINT2("subfe. ",i,j,r0));
    ITERATE1(__asm__ (clearCrXer "subfme  %0,%4"    getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);   PRINT1("subfme ",i,r0));
    ITERATE1(__asm__ (clearCrXer "subfme. %0,%4"    getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);   PRINT1("subfme.",i,r0));
    ITERATE1(__asm__ (clearCrXer "subfze  %0,%4"    getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);   PRINT1("subfze ",i,r0));
    ITERATE1(__asm__ (clearCrXer "subfze. %0,%4"    getCrXer : "=r"(r0), "=r"(_xer), "=r"(_cr) : "r"(zero), "r"(r1), "i"(-1) : clobberCrXer);   PRINT1("subfze.",i,r0));

    return 0;
}
