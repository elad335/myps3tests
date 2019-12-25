
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/spu_thread_group.h>
#include <sys/spu_thread.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>
#include <string>
#include <spu_printf.h>
#include <sys/timer.h>

#include "../../ppu_tests/ppu_header.h"

/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];

int main(void)
{
	sys_memory_t shm;
	sys_addr_t addr_sh;

	ENSURE_OK(sys_mmapper_allocate_address(0x10000000, SYS_MEMORY_PAGE_SIZE_64K, 0x10000000, &addr_sh));

	const sys_addr_t addr_shm0 = addr_sh + 0x10000, addr_shm1 = addr_shm0 + 0x10000;
 
	ENSURE_OK(sys_mmapper_allocate_memory(0x10000, SYS_MEMORY_PAGE_SIZE_64K, &shm));
	ENSURE_OK(sys_mmapper_map_memory(addr_shm0, shm, SYS_MEMORY_PROT_READ_WRITE));
	ENSURE_OK(sys_mmapper_map_memory(addr_shm1, shm, SYS_MEMORY_PROT_READ_WRITE));

	if (true)
	{
		const u32 old = __lwarx(ptr_caste(addr_shm0, u32));
		const bool success = __stwcx(ptr_caste(addr_shm1, u32), old) != 0;
		printf("PPU atomic operation status: %s\n",(success) ? "success" : "failure");
	}

	sys_spu_initialize(1, 0);

	sys_spu_thread_group_t grp_id;
	int prio = 100;
	sys_spu_thread_group_attribute_t grp_attr;
	
	sys_spu_thread_group_attribute_initialize(grp_attr);
	sys_spu_thread_group_attribute_name(grp_attr, "test spu grp");
	ENSURE_OK(sys_spu_thread_group_create(&grp_id, 1, prio, &grp_attr));

	sys_spu_image_t img;
	ENSURE_OK(sys_spu_image_import(&img, +_binary_test_spu_spu_out_start, SYS_SPU_IMAGE_DIRECT));

	sys_spu_thread_t thr_id;
	sys_spu_thread_attribute_t thr_attr;
	
	sys_spu_thread_attribute_initialize(thr_attr);
	sys_spu_thread_attribute_name(thr_attr, "test spu thread");
	sys_spu_thread_argument_t thr_args;
	thr_args.arg1 = addr_shm0;
	thr_args.arg2 = addr_shm1;
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

	sys_timer_sleep(1);
	printf("sample finished.");
	return 0;
}