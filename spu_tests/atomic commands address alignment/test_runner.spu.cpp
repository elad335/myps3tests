#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>
#include <string.h>

#include "../spu_header.h"

static u64 rdata[16] __attribute__((aligned(256))) = {};

int main(u64 raddr, u64, u64, u64) {

	spu_printf("0x%x\n", raddr);
	spu_printf("ok\n");

	spu_writech(MFC_EAL, raddr + 0x7F); // Unaligned!!
	spu_writech(MFC_EAH, 0);
	spu_writech(MFC_LSA , int_cast(&rdata[0]) + 0x7F); // Unaligned!!
	spu_writech(MFC_Size, 0x80);
	spu_writech(MFC_Cmd, 0xD0); // GETLLAR
	spu_readch(MFC_RdAtomicStat); fsync();

	spu_printf("rdata:\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	"%llx\n"
	, *ptr_caste(rdata + 0, u64)
	, *ptr_caste(rdata + 1, u64)
	, *ptr_caste(rdata + 2, u64)
	, *ptr_caste(rdata + 3, u64)
	, *ptr_caste(rdata + 4, u64)
	, *ptr_caste(rdata + 5, u64)
	, *ptr_caste(rdata + 6, u64)
	, *ptr_caste(rdata + 7, u64)
	, *ptr_caste(rdata + 8, u64)
	, *ptr_caste(rdata + 9, u64)
	, *ptr_caste(rdata + 10, u64)
	, *ptr_caste(rdata + 11, u64)
	, *ptr_caste(rdata + 12, u64)
	, *ptr_caste(rdata + 13, u64)
	, *ptr_caste(rdata + 14, u64)
	, *ptr_caste(rdata + 15, u64));
	fsync();

	spu_writech(MFC_EAL, raddr); // aligned
	spu_writech(MFC_EAH, 0);
	spu_writech(MFC_LSA , int_cast(&rdata[0])); // aligned
	spu_writech(MFC_Size, 0x80);
	spu_writech(MFC_Cmd, 0xB4); // PUTLLC
	spu_printf("reservation status: 0x%x", spu_readch(MFC_RdAtomicStat));
	fsync();

	sys_spu_thread_exit(0);
	return 0;
}