
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/spu_thread_group.h>
#include <sys/spu_thread.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_syscache.h>
#include <cell/sysmodule.h>
#include <cell/atomic.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/syscall.h>

#include <spu_printf.h>
#include <functional>

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

typedef int32_t s32;

#define int_cast(x) reinterpret_cast<uptr>(x)
#define ptr_cast(x) reinterpret_cast<void*>(x)

/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];
static u32 rdata[32] __attribute__((aligned(128))) = {0};

void threadEntry(u64)
{
    while(true)
    {
        rdata[31] = 0;
        sys_timer_usleep(300);
    }
}

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
    thr_args.arg1 = int_cast(&rdata[0]);
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

	// start thread 
	{
	    int result;
	    sys_ppu_thread_t m_tid1;

		result = sys_ppu_thread_create(
			&m_tid1,
			threadEntry,
			0,
			1002, 0x10000,
			SYS_PPU_THREAD_CREATE_JOINABLE,
			"test");
		if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
		}
	}	

    ret = sys_spu_thread_group_start(grp_id);    
    if (ret != CELL_OK) {
        printf("sys_spu_thread_group_start: %d\n", ret);
        return ret;
    }

    asm volatile ("nop;eieio;sync");
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
