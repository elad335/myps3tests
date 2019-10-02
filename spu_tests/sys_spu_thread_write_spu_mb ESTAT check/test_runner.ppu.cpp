
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

typedef uint32_t u32;

// Send SPU count in DMA transfer as we cant use spu printf since it uses the mailbox
static u32 mailbox_count __attribute__((aligned(16))) = -1;

int main(void)
{
	int ret;
	
	sys_spu_initialize(1, 0);

	sys_spu_thread_group_t grp_id;
	sys_spu_thread_group_attribute_t grp_attr;
	
	sys_spu_thread_group_attribute_initialize(grp_attr);
	sys_spu_thread_group_attribute_name(grp_attr, "test spu grp");
	ret = sys_spu_thread_group_create(&grp_id, 1, 100, &grp_attr);
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

	// Set mailbox count var address in args
	thr_args.arg1 = reinterpret_cast<uintptr_t>(&mailbox_count);

	ret = sys_spu_thread_initialize(&thr_id, grp_id, 0, &img, &thr_attr, &thr_args);
	if (ret != CELL_OK) {
		printf("sys_spu_thread_initialize: %d\n", ret);
		return ret;
	}

	spu_printf_initialize(1000, NULL);
	spu_printf_attach_group(grp_id);

	// Fill the mailbox. right before spu thread starts
	// This tests the error code returned when group status is INITIALIZED
	printf("sys_spu_thread_write_spu_mb returned: 0x%x\n", sys_spu_thread_write_spu_mb(thr_id, 0x1234));

	// Start the thread
	sys_spu_thread_group_start(grp_id);	
	asm volatile ("twi 0x10, 3, 0");

	int cause;
	int status;
	sys_spu_thread_group_join(grp_id, &cause, &status);
	asm volatile ("twi 0x10, 3, 0");

	printf("mailbox count=0x%x\n", mailbox_count);

	// 'Restart' the thread
	sys_spu_thread_group_start(grp_id);	
	asm volatile ("twi 0x10, 3, 0");

	sys_spu_thread_group_join(grp_id, &cause, &status);
	asm volatile ("twi 0x10, 3, 0");

	printf("mailbox count after restart=0x%x\n", mailbox_count);

	spu_printf_detach_group(grp_id);
	spu_printf_finalize();

	sys_spu_thread_group_destroy(grp_id);
	asm volatile ("twi 0x10, 3, 0");

	printf("sample finished.\n");

	return 0;
}
