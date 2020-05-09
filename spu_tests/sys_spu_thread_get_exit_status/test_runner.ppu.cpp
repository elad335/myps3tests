
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

int main(void)
{
	sys_spu_initialize(1, 1);

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
	sys_spu_thread_argument_t thr_args = {};
	ENSURE_OK(sys_spu_thread_initialize(&thr_id, grp_id, 0, &img, &thr_attr, &thr_args));

	ENSURE_OK(spu_printf_initialize(1000, NULL));
	ENSURE_OK(spu_printf_attach_group(grp_id));

	int cause;
	int status;
	int thr_code;

	for (u32 i = 0; i < 8; i++)
	{
		cause = 1u<<31;
		status = 1u<<31;
		thr_code = 1u<<31;
		thr_args.arg1 = i;
		ENSURE_OK(sys_spu_thread_set_argument(thr_id, &thr_args));
		ENSURE_OK(sys_spu_thread_group_start(grp_id));

		if (i >= 7 - 1)
		{
			justFunc(sys_spu_thread_get_exit_status, thr_id, &thr_code);
			if (i % 2 == 0) ENSURE_OK(sys_spu_thread_group_terminate(grp_id, i));
		}

		ENSURE_OK(sys_spu_thread_group_join(grp_id, &cause, &status));

		if (i < 7 - 1)
		{
			justFunc(sys_spu_thread_get_exit_status, thr_id, &thr_code);
		}

		printf("thread status: 0x%x, group status: 0x%x, cause: 0x%x\n", thr_code, status, cause);
	}

	spu_printf_detach_group(grp_id);
	spu_printf_finalize();

	ENSURE_OK(sys_spu_thread_group_destroy(grp_id));
	return 0;
}
