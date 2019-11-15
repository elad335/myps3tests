#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <cstring>
#include <math.h>
#include <float.h>

#include "../spu_header.h"

// Test values
static const volatile f32 Values[] =
{
	// Denormals and zero
	bit_cast<f32, u32>(0), 
	bit_cast<f32, u32>(2), 
	bit_cast<f32, u32>(16),
	bit_cast<f32, u32>(0x007fffff),
	// Minus denormal
	bit_cast<f32, u32>(0x807fffff),
	// Random numbers
	bit_cast<f32, u32>(0x00033333),
	bit_cast<f32, u32>(0x00fffff0),
	bit_cast<f32, u32>(0x10000000),
	bit_cast<f32, u32>(0x12345678),
	bit_cast<f32, u32>(0x42385722),
	bit_cast<f32, u32>(0x70000000),
	bit_cast<f32, u32>(0x72233411),
	// Generic floats
	-0.0,  1.0, -1.0,  1.5, -1.5,
	1.6, -1.6,  1.4, -1.4,  2.0, 2.00043245,
	4.0, -10000000.4, 20000000.0, -20000.5, 20000.6,
	std::pow(2.f, 126),
	bit_cast<f32, u32>(0x00800000),
	// Smax+-
	bit_cast<f32, u32>(0x7fffffff),
	bit_cast<f32, u32>(0xffffffff),
};

void print_res(const char* inst, f32 opr, f32 res)
{
	spu_printf("%s[%f (0x%x)] result = %f (0x%x)\n", inst, opr, bit_cast<u32>(opr), res, bit_cast<u32>(res));
}

int main()
{
	f32 res;

	for (u32 i = 0; i < sizeof(Values) / sizeof(f32); i++)
	{
		res = spu_op2_f(fi, Values[i], spu_op1_f(frest, Values[i]));
		print_res("frest_fi", Values[i], res);
	}

	for (u32 i = 0; i < sizeof(Values) / sizeof(f32); i++)
	{
		res = spu_op2_f(fi, Values[i], spu_op1_f(frsqest, Values[i]));
		print_res("frsqest_fi", Values[i], res);
	}

	for (u32 i = 0; i < sizeof(Values) / sizeof(f32); i++)
	{
		res = spu_op1_f(frest, Values[i]);
		print_res("frest", Values[i], res);
	}

	for (u32 i = 0; i < sizeof(Values) / sizeof(f32); i++)
	{
		res = spu_op1_f(frsqest, Values[i]);
		print_res("frsqest", Values[i], res);
	}

	sys_spu_thread_exit(0);
	return 0;
}