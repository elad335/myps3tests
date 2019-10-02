
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

int main(void)
{
	int ret;
	
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
	uint32_t i = 0xfff;
	uint32_t t = 0;
	uint32_t cell = 0;
	printf("hmm\n");
	//while ((cell = sys_spu_thread_get_exit_status(thr_id, &i)) != 0){t++;}
	//printf("sucess%x , 0x%x\n", i, sys_spu_thread_get_exit_status(thr_id, &i));
	int cause;
	int status;
	ret = sys_spu_thread_group_join(grp_id, &cause, &status);
	if (ret != CELL_OK) {
		printf("sys_spu_thread_group_join: %d\n", ret);
		return ret;
	}

	printf("sucess%x\n", sys_spu_thread_get_exit_status(thr_id, &i));
	printf("%x\n",i);
	printf("sucess%x\n", sys_spu_thread_get_exit_status(thr_id, &i));
	printf("%x\n",i);
	spu_printf_detach_group(grp_id);
	spu_printf_finalize();

	ret = sys_spu_thread_group_destroy(grp_id);
	if (ret != CELL_OK) {
		printf("sys_spu_thread_group_destroy: %d\n", ret);
		return ret;
	}

	return 0;
}
