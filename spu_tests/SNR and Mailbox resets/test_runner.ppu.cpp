
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

volatile u32 ch_cnt = 0;

int main(void)
{
	ENSURE_OK(sys_spu_initialize(1, 0));

	// For group join
	int cause;
	int status;

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
	thr_args.arg2 = int_cast(&ch_cnt);
	thr_args.arg3 = 0;
	ENSURE_OK(sys_spu_thread_initialize(&thr_id, grp_id, 0, &img, &thr_attr, &thr_args));

	printf("Testing SNR registers reset...\n");

	// Write to SNR1
	ENSURE_OK(sys_spu_thread_write_snr(thr_id, 0, 0));

	// 1 for snr, 2 for mailbox
	thr_args.arg1 = 1;
	ENSURE_OK(sys_spu_thread_set_argument(thr_id, &thr_args));

	ENSURE_OK(sys_spu_thread_group_start(grp_id))
	ENSURE_OK(sys_spu_thread_group_join(grp_id, &cause, &status));

	// Test count after group start
	printf("SNR1 channel count after sys_spu_thread_group_start = 0x%x\n", ch_cnt);

	ENSURE_OK(sys_spu_thread_group_start(grp_id))
	ENSURE_OK(sys_spu_thread_group_join(grp_id, &cause, &status));

	// Test count after group exit as well
	printf("SNR1 channel count after sys_spu_thread_exit = 0x%x\n", ch_cnt);


	printf("Testing Mailbox reset...\n");

	// Write to mailbox until full
	for (u32 i = 0; i < 4; i++)
		ENSURE_OK(sys_spu_thread_write_spu_mb(thr_id, 0));

	// 1 for snr, 2 for mailbox
	thr_args.arg1 = 2;
	ENSURE_OK(sys_spu_thread_set_argument(thr_id, &thr_args));

	ENSURE_OK(sys_spu_thread_group_start(grp_id))
	ENSURE_OK(sys_spu_thread_group_join(grp_id, &cause, &status));

	// Test count after group start
	printf("Mailbox channel count after sys_spu_thread_group_start = 0x%x\n", ch_cnt);

	ENSURE_OK(sys_spu_thread_group_start(grp_id))
	ENSURE_OK(sys_spu_thread_group_join(grp_id, &cause, &status));

	// Test count after group exit as well
	printf("Mailbox channel count after sys_spu_thread_exit = 0x%x\n", ch_cnt);

	// Write to mailbox until full again
	for (u32 i = 0; i < 4; i++)
		ENSURE_OK(sys_spu_thread_write_spu_mb(thr_id, 0));

	// Set to wait for external termination
	thr_args.arg3 = 1;
	ENSURE_OK(sys_spu_thread_set_argument(thr_id, &thr_args));

	ENSURE_OK(sys_spu_thread_group_start(grp_id))
	ENSURE_OK(sys_spu_thread_group_terminate(grp_id, 0)); // Terminate externally

	// Restore SPU thread manual exit
	thr_args.arg3 = 0;
	ENSURE_OK(sys_spu_thread_set_argument(thr_id, &thr_args));

	ENSURE_OK(sys_spu_thread_group_start(grp_id))
	ENSURE_OK(sys_spu_thread_group_join(grp_id, &cause, &status));

	// Test count after an external termination (either from sys_spu_thread_group_exit or sys_spu_thread_group_terminate)
	printf("Mailbox channel count after sys_spu_thread_group_terminate = 0x%x\n", ch_cnt);

	ENSURE_OK(sys_spu_thread_group_destroy(grp_id));
	return 0;
}
