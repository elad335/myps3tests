#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>

typedef uint32_t u32;
typedef float f32;


inline void sync() { asm volatile ("lnop;sync;syncc;dsync;stop 0x100"); };
#define assert(); asm volatile("stop 0x3fff");

enum // Floats in Hex
{
	NearInf = 0x7f800000, 
};

int main()
{
	register vec_int4 reg1 asm ("111");
	register vec_int4 reg2 asm ("112");
	asm volatile (
		"il $111, 0x7f8;"
		"shli $111, $111, 20;"
		"fma $112, $111,$111, $111;"
	);
	spu_printf("reg[0] value = 0x%x", spu_extract(reg2, 0));
	sys_spu_thread_exit(0);
	return 0;
}