
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

void test(u64 _exit)
{
	u8 buf[16];
	memset(buf, 127, 16);
	const vec_u32 c = bit_cast<vec_u32>(__builtin_altivec_mfvscr());

	// Set SAT bit
	const vec_s8 ugh = 	__builtin_vec_vaddsbs(bit_cast<vec_s8>(buf), bit_cast<vec_s8>(buf));
	memcpy(&buf, &ugh, 13);
	const vec_u32 d = bit_cast<vec_u32>(__builtin_altivec_mfvscr());
	print_obj(c);
	print_obj(d);
	if (_exit) sys_ppu_thread_exit(0);
}

int main(void)
{
	test(0);
	printf("END MAIN THREAD\n");

	u64 m_tid;
	ENSURE_OK(sys_ppu_thread_create(
			&m_tid,
			test,
			1,
			1002, 0x10000,
			SYS_PPU_THREAD_CREATE_JOINABLE,
			"test"));

	ENSURE_VAL(sys_ppu_thread_join(m_tid, NULL), EFAULT);
	sys_timer_sleep(1);
	printf("END HOST THREAD");
	return 0;
}
