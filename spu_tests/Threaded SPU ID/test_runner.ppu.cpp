
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/spu_thread_group.h>
#include <sys/spu_thread.h>
#include <sys/ppu_thread.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>

#include <cell/dma/types.h>
#include <cell/dma.h>
#include <sys/dbg.h>
#include <sys/timer.h>
#include <cell/sysmodule.h> 

#include <spu_printf.h>

#include "../../ppu_tests/ppu_header.h"

/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];

sys_spu_thread_t thr_id[6] = {0};

int main(void)
{
	sys_spu_initialize(6, 0);

	sys_spu_thread_group_t grp_id[2];
	int prio = 100;
	sys_spu_thread_group_attribute_t grp_attr;
	
	sys_spu_thread_group_attribute_initialize(grp_attr);
	sys_spu_thread_group_attribute_name(grp_attr, "test spu grp");
	ENSURE_OK(sys_spu_thread_group_create(grp_id + 0, 6, prio, &grp_attr));

	printf("SPU Group Id = 0x%08x\n", grp_id[0]);

	sys_spu_image_t img;
	ENSURE_OK(sys_spu_image_import(&img, (void*)_binary_test_spu_spu_out_start, SYS_SPU_IMAGE_DIRECT));

	sys_spu_thread_attribute_t thr_attr;

	sys_spu_thread_attribute_initialize(thr_attr);
	sys_spu_thread_attribute_name(thr_attr, "test spu thread");
	sys_spu_thread_argument_t thr_args;

	ENSURE_OK(sys_spu_thread_initialize(thr_id + 0, grp_id[0], 0, &img, &thr_attr, &thr_args));
	ENSURE_OK(sys_spu_thread_initialize(thr_id + 1, grp_id[0], 2, &img, &thr_attr, &thr_args));
	ENSURE_OK(sys_spu_thread_initialize(thr_id + 2, grp_id[0], 15, &img, &thr_attr, &thr_args));
	ENSURE_OK(sys_spu_thread_initialize(thr_id + 3, grp_id[0], 4, &img, &thr_attr, &thr_args));
	ENSURE_OK(sys_spu_thread_initialize(thr_id + 4, grp_id[0], 8, &img, &thr_attr, &thr_args));
	ENSURE_OK(sys_spu_thread_initialize(thr_id + 5, grp_id[0], 9, &img, &thr_attr, &thr_args));

	for (u32 i = 0; i < 6; i++)
	{
		printf("SPU Thread(%d) Id = 0x%08x\n", i, thr_id[i]);
	}

	printf("Testing thr_id[4]...\n");

	sysCell(spu_thread_group_start, thr_id[4]);
	sysCell(spu_thread_group_terminate, thr_id[4], 0);

	ENSURE_OK(sys_spu_thread_group_destroy(grp_id[0]));

	ENSURE_OK(sys_spu_thread_group_create(grp_id + 0, 3, prio, &grp_attr));
	ENSURE_OK(sys_spu_thread_group_create(grp_id + 1, 3, prio, &grp_attr));

	for (u32 i = 0; i < 2; i++)
	{
		printf("SPU Group(%d) Id = 0x%08x\n", i, grp_id[i]);
	}

	ENSURE_OK(sys_spu_thread_initialize(thr_id + 0, grp_id[0], 0, &img, &thr_attr, &thr_args));
	ENSURE_OK(sys_spu_thread_initialize(thr_id + 1, grp_id[0], 2, &img, &thr_attr, &thr_args));
	ENSURE_OK(sys_spu_thread_initialize(thr_id + 2, grp_id[0], 15, &img, &thr_attr, &thr_args));
	ENSURE_OK(sys_spu_thread_initialize(thr_id + 3, grp_id[1], 0, &img, &thr_attr, &thr_args));
	ENSURE_OK(sys_spu_thread_initialize(thr_id + 4, grp_id[1], 2, &img, &thr_attr, &thr_args));
	ENSURE_OK(sys_spu_thread_initialize(thr_id + 5, grp_id[1], 15, &img, &thr_attr, &thr_args));

	for (u32 i = 0; i < 6; i++)
	{
		printf("SPU Thread(group=%d, %d) Id = 0x%08x\n", i / 3, i % 3, thr_id[i]);
	}

	ENSURE_OK(sys_spu_thread_group_destroy(grp_id[0]));
	ENSURE_OK(sys_spu_thread_group_destroy(grp_id[1]));

	return 0;
}
