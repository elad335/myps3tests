
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

static const u32 spu_q_number = -1u;//0x12345678;

int main(void)
{
	sys_spu_initialize(1, 0);

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

	thr_args.arg1 = spu_q_number;
	ENSURE_OK(sys_spu_thread_initialize(&thr_id, grp_id, 0, &img, &thr_attr, &thr_args));

	ENSURE_OK(spu_printf_initialize(1000, NULL));
	ENSURE_OK(spu_printf_attach_group(grp_id));

	sys_event_queue_t queue;
	sys_event_queue_attribute_t queue_attr;
	queue_attr.attr_protocol = SYS_SYNC_PRIORITY;
	queue_attr.type = SYS_SPU_QUEUE;
	std::memset(queue_attr.name, 0, 8);

	// Create queue, bind, destroy it
	sysCell(event_queue_create, &queue, &queue_attr, SYS_EVENT_QUEUE_LOCAL, 1);
	sysCell(spu_thread_bind_queue, thr_id, queue, spu_q_number);
	sysCell(event_queue_destroy, queue, 0);

	// Test if it is ok to unbind after the original queue was destroyed
	const u32 ret_unbind = sysCell(spu_thread_unbind_queue, thr_id, spu_q_number);

	sysCell(event_queue_create, &queue, &queue_attr, SYS_EVENT_QUEUE_LOCAL, 1);
	sysCell(spu_thread_bind_queue, thr_id, queue, spu_q_number);
	sysCell(event_queue_destroy, queue, 0);

	// Test if it is ok to bind new queue after the previous queue was destroyed
	sysCell(event_queue_create, &queue, &queue_attr, SYS_EVENT_QUEUE_LOCAL, 1);
	const u32 ret_bind = sysCell(spu_thread_bind_queue, thr_id, queue, spu_q_number);

	if (ret_bind != CELL_OK)
	{
		if (ret_unbind != CELL_OK)
		{
			// Can't rebind queue in such case
			printf("Unhandled fatal case occured!\n");
			return ret_unbind;
		}

		ENSURE_OK(sys_spu_thread_unbind_queue(thr_id, spu_q_number));
		sysCell(spu_thread_bind_queue, thr_id, queue, spu_q_number);
	}
	else
	{
		printf("Reusing binded queue.\n");
	}

	sysCell(event_queue_destroy, queue, 0);
	sysCell(spu_thread_unbind_queue, thr_id, spu_q_number);

	// Bind and destroy queue, use that for SPU usage
	ENSURE_OK(sys_event_queue_create(&queue, &queue_attr, SYS_EVENT_QUEUE_LOCAL, 1));
	ENSURE_OK(sys_spu_thread_bind_queue(thr_id, queue, spu_q_number));
	ENSURE_OK(sys_event_queue_destroy(queue, 0));

	sys_timer_sleep(1);

	ENSURE_OK(sys_spu_thread_group_start(grp_id));
	int cause;
	int status;
	ENSURE_OK(sys_spu_thread_group_join(grp_id, &cause, &status));

	spu_printf_detach_group(grp_id);
	spu_printf_finalize();

	ENSURE_OK(sys_spu_thread_group_destroy(grp_id));

	return 0;
}
