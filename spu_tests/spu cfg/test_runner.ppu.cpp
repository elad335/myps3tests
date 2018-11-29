
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
    
	sys_spu_initialize(1, 0);

    ret = sys_spu_thread_set_spu_cfg(-1u /* No spu thread */, 4 /*Invalid value*/);
    printf("sys_spu_thread_set_spu_cfg returned: 0x%x\n", ret);

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

    // Set spu cfg
    sys_spu_thread_set_spu_cfg(thr_id, 3);

    ret = sys_spu_thread_group_start(grp_id);    
    if (ret != CELL_OK) {
        printf("sys_spu_thread_group_start: %d\n", ret);
        return ret;
    }

    // Check value after group_start
    uint64_t value;
    sys_spu_thread_get_spu_cfg(thr_id, &value);
    printf("sys_spu_thread_get_spu_cfg value after group_start: 0x%x\n", value);

    int cause;
    int status;
    ret = sys_spu_thread_group_join(grp_id, &cause, &status);
    if (ret != CELL_OK) {
        printf("sys_spu_thread_group_join: %x\n", ret);
        return ret;
    }

    ret = sys_spu_thread_group_destroy(grp_id);
    if (ret != CELL_OK) {
        printf("sys_spu_thread_group_destroy: %d\n", ret);
        return ret;
    }

	return 0;
}
