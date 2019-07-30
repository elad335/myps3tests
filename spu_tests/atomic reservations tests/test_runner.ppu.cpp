
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

static u32 rdata[256] __attribute__((aligned(128))) = {0};

#define BEGIN_SCOPE do{
#define CLOSE_SCOPE(expr) }while(expr)

static void ppu_reservation_tests()
{
    rdata[0] = 0;
    rdata[1] = 1;
    fsync();

    BEGIN_SCOPE;
    __lwarx(&rdata[0]);
    bool success = __stwcx(&rdata[1], rdata[1]) != 0;
    printf("PPU - Performing atomic store on a nieghbor address of load's\n status:%s\n", success ? "true" : "false");
    CLOSE_SCOPE(0);

    BEGIN_SCOPE;
    __ldarx(ptr_caste(&rdata[0], u64));
    bool success = __stwcx(&rdata[0], rdata[0]) != 0;
    printf("PPU - Performing u64 to u32 atomic op\n status:%s\n", success ? "true" : "false");
    CLOSE_SCOPE(0);

    BEGIN_SCOPE;
    __lwarx(&rdata[0]);
    bool success = __stdcx(ptr_caste(&rdata[0], u64), *ptr_caste(&rdata[0], u64)) != 0;
    printf("PPU - Performing u32 to u64 atomic op\n status:%s\n", success ? "true" : "false");
    CLOSE_SCOPE(0);

    BEGIN_SCOPE;
    const u32 old = __lwarx(&rdata[0]);
    rdata[0] = old + 1;
    fsync();
    bool success = __stwcx(&rdata[0], rdata[0]) != 0;
    printf("PPU - Performing atomic store after a self written non-atomic store\n status:%s\n", success ? "true" : "false");
    rdata[0] = old - 1;
    fsync();
    CLOSE_SCOPE(0);

    rdata[0] = 0;
    rdata[1] = 0;
    fsync();
    sys_timer_sleep(1);
} 

int main(void)
{
    ppu_reservation_tests();

	int ret;
	sys_spu_initialize(1, 0);

    sys_spu_thread_group_t grp_id;
    int prio = 100;
    sys_spu_thread_group_attribute_t grp_attr;
    
    sys_spu_thread_group_attribute_initialize(grp_attr);
    sys_spu_thread_group_attribute_name(grp_attr, "test spu grp");
    ret = sys_spu_thread_group_create(&grp_id, 2, prio, &grp_attr);
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
    thr_args.arg1 = reinterpret_cast<uptr>(&rdata[0]);
    thr_args.arg2 = 0;
    ret = sys_spu_thread_initialize(&thr_id, grp_id, 0, &img, &thr_attr, &thr_args);
    if (ret != CELL_OK) {
        printf("sys_spu_thread_initialize: %d\n", ret);
        return ret;
    }

	thr_args.arg2 = 1;
	ret = sys_spu_thread_initialize(&thr_id, grp_id, 1, &img, &thr_attr, &thr_args);
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

    sys_timer_sleep(1);
	printf("sample finished");
	return 0;
}
