
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/spu_thread_group.h>
#include <sys/spu_thread.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>

#include <spu_printf.h>

#include "../../ppu_tests/ppu_header.h"

/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];

static u64 reserv[16] __attribute__((aligned(256))) = {1, 2, 3, 4, 5, 6, 7, 8, 9 ,10, 11, 12, 13, 14, 15, 16};

int main(void)
{
	int ret;

	u64 ctx_addr;
	{
		system_call_3(SYS_RSX_DEVICE_MAP,  (uint64_t)(&ctx_addr), 0, 8);
		int ret = (int)(p1);
		printf("ret is 0x%x, addr = 0x%llx\n", ret, ctx_addr);
	}
	u32 mem_handle;
	{
		u64 mem_addr;
		system_call_7(SYS_RSX_MEMORY_ALLOCATE, (uintptr_t)(&mem_handle), (uintptr_t)(&mem_addr), 0xf900000, 0x80000, 0x300000, 0xf, 0x8); //size = 0xf900000, 249 mb
		int ret = (int)(p1);
		printf("ret is 0x%x, mem_handle= 0x%x, mem_addr= 0x%llx\n", ret, mem_handle, mem_addr);
	}

	u32 context_id;
	u64 lpar_dma_control;
	u64 lpar_driver_info;
	u64 lpar_reports;
	{
		system_call_6(SYS_RSX_CONTEXT_ALLOCATE, (uintptr_t)(&context_id), (uintptr_t)(&lpar_dma_control), (uintptr_t)(&lpar_driver_info), (uintptr_t)(&lpar_reports), mem_handle, 0x820);
		int ret = (int)(p1);
		printf("ret is 0x%x\n", ret);
	}
	
	ENSURE_OK(sys_spu_initialize(1, 0));

	sys_spu_thread_group_t grp_id;
	int prio = 100;
	sys_spu_thread_group_attribute_t grp_attr;
	
	sys_spu_thread_group_attribute_initialize(grp_attr);
	sys_spu_thread_group_attribute_name(grp_attr, "test spu grp");
	ENSURE_OK(sys_spu_thread_group_create(&grp_id, 1, prio, &grp_attr));

	sys_spu_image_t img;
	ENSURE_OK(sys_spu_image_import(&img, (void*)_binary_test_spu_spu_out_start, SYS_SPU_IMAGE_DIRECT));

	sys_spu_thread_t thr_id;
	sys_spu_thread_attribute_t thr_attr;
	
	sys_spu_thread_attribute_initialize(thr_attr);
	sys_spu_thread_attribute_name(thr_attr, "test spu thread");
	sys_spu_thread_argument_t thr_args;

	thr_args.arg1 = ctx_addr;
	ENSURE_OK(sys_spu_thread_initialize(&thr_id, grp_id, 0, &img, &thr_attr, &thr_args));

	ENSURE_OK(spu_printf_initialize(1000, NULL));
	ENSURE_OK(spu_printf_attach_group(grp_id));

	ENSURE_OK(sys_spu_thread_group_start(grp_id));

	int cause;
	int status;
	ENSURE_OK(sys_spu_thread_group_join(grp_id, &cause, &status));

	spu_printf_detach_group(grp_id);
	spu_printf_finalize();

	ENSURE_OK(sys_spu_thread_group_destroy(grp_id));

	return 0;
}
