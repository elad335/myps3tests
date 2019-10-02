
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/spu_thread_group.h>
#include <sys/spu_thread.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>

#include <spu_printf.h>


/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];

typedef uintptr_t uptr;
typedef uint32_t u32;
typedef uint64_t u64;

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
	
	ret = sys_spu_initialize(1, 0);
	if (ret != CELL_OK) {
		printf("sys_spu_initialize failed: %d\n", ret);
		return ret;
	}

	sys_spu_thread_group_t grp_id;
	int prio = 100;
	sys_spu_thread_group_attribute_t grp_attr;
	
	sys_spu_thread_group_attribute_initialize(grp_attr);
	sys_spu_thread_group_attribute_name(grp_attr, "test spu grp");
	ret = sys_spu_thread_group_create(&grp_id, 1, prio, &grp_attr);
	if (ret != CELL_OK) {
		printf("spu_thread_group_create failed: %d\n", ret);
		return ret;
	}

	sys_spu_image_t img;

	ret = sys_spu_image_import(&img, (void*)_binary_test_spu_spu_out_start, SYS_SPU_IMAGE_DIRECT );
	if (ret != CELL_OK) {
		printf("sys_spu_image_import: %d\n", ret);
		return ret;
	}

	sys_spu_thread_t thr_id;
	sys_spu_thread_attribute_t thr_attr;
	
	sys_spu_thread_attribute_initialize(thr_attr);
	sys_spu_thread_attribute_name(thr_attr, "test spu thread");
	sys_spu_thread_argument_t thr_args;

	thr_args.arg1 = ctx_addr;
	ret = sys_spu_thread_initialize(&thr_id, grp_id, 0, &img, &thr_attr, &thr_args);
	if (ret != CELL_OK) {
		printf("sys_spu_thread_initialize: %d\n", ret);
		return ret;
	}

	ret = spu_printf_initialize(1000, NULL);
	if (ret != CELL_OK) {
		printf("spu_printf_initialize failed %x\n", ret);
		exit(-1);
	}

	ret = spu_printf_attach_group(grp_id);
	if (ret != CELL_OK) {
		printf("spu_printf_attach_group failed %x\n", ret);
		exit(-1);
	}

	ret = sys_spu_thread_group_start(grp_id);	
	if (ret != CELL_OK) {
		printf("sys_spu_thread_group_start: %d\n", ret);
		return ret;
	}

	int cause;
	int status;
	ret = sys_spu_thread_group_join(grp_id, &cause, &status);
	if (ret != CELL_OK) {
		printf("sys_spu_thread_group_join: %d\n", ret);
		return ret;
	}

	spu_printf_detach_group(grp_id);
	spu_printf_finalize();

	ret = sys_spu_thread_group_destroy(grp_id);
	if (ret != CELL_OK) {
		printf("sys_spu_thread_group_destroy: %d\n", ret);
		return ret;
	}

	return 0;
}
