
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


int main(void)
{
	vector float vec_nan0 = bit_cast<vector float>((vector int){0x7fc00001, 0x7fc00001, 0x7fc00001, 0x7fc00001});
	vector float vec_nan1 = bit_cast<vector float>((vector int){0x7fc00002, 0x7fc00002, 0x7fc00002, 0x7fc00002});
	vector float vec_zero = bit_cast<vector float>(vec_splat_s32(0));

	vector float vecs[3] =
	{
		vec_nan0, vec_nan1, vec_zero
	};

	for (u32 i = 0; i < 3;i++)
		for (u32 i1 = 0; i1 < 3;i1++)
			for (u32 i2 = 0; i2 < 3;i2++)
			{
				const vector float res = vec_madd(vecs[i], vecs[i1], vecs[i2]);
				printf("index: %d %d %d: ", i, i1, i2);
				print_obj(res);
			}

	return 0;
}
