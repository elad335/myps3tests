
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

sys_spu_thread_t thr_id = 0;
s64 srr0 = -1;

void callback(
    uint64_t,
    sys_ppu_thread_t,
    uint64_t
)
{
	sys_dbg_spu_thread_context2_t ctxt_spu;
	if (int err = sys_dbg_read_spu_thread_context2(thr_id, &ctxt_spu))
	{
		if (err >= -1)
		{
			// Unreachable
			err = -2;
		}

		store_vol(srr0, err);
	}
	else
	{
		store_vol(srr0, ctxt_spu.srr0);
	}
}

void signal_thread(u64)
{
	if (int err = sys_dbg_signal_to_ppu_exception_handler(0))
	{
		printf("sys_dbg_signal_to_ppu_exception_handler(error=0x%x)\n", err);
	}

	sys_ppu_thread_exit(0);
}

int main(void)
{
    cellSysmoduleLoadModule(CELL_SYSMODULE_LV2DBG);
	ENSURE_OK(sys_dbg_initialize_ppu_exception_handler(0));
	ENSURE_OK(sys_dbg_register_ppu_exception_handler(&callback, SYS_DBG_SPU_THREAD_GROUP_STOP | SYS_DBG_SYSTEM_SPU_THREAD_GROUP_NOT_STOP));

	sys_spu_initialize(1, 0);

	sys_spu_thread_group_t grp_id;
	int prio = 100;
	sys_spu_thread_group_attribute_t grp_attr;
	
	sys_spu_thread_group_attribute_initialize(grp_attr);
	sys_spu_thread_group_attribute_name(grp_attr, "test spu grp");
	ENSURE_OK(sys_spu_thread_group_create(&grp_id, 1, prio, &grp_attr));

	sys_spu_image_t img;
	ENSURE_OK(sys_spu_image_import(&img, (void*)_binary_test_spu_spu_out_start, SYS_SPU_IMAGE_DIRECT));

	sys_spu_thread_attribute_t thr_attr;

	sys_spu_thread_attribute_initialize(thr_attr);
	sys_spu_thread_attribute_name(thr_attr, "test spu thread");
	sys_spu_thread_argument_t thr_args;
	thr_args.arg1 = (u32)&srr0;
	ENSURE_OK(sys_spu_thread_initialize(&thr_id, grp_id, 0, &img, &thr_attr, &thr_args));

	ENSURE_OK(spu_printf_initialize(1000, NULL));
	ENSURE_OK(spu_printf_attach_group(grp_id));

	ENSURE_OK(sys_spu_thread_group_start(grp_id));

	s64 _srr0 = -1;
	while (_srr0 == -1)
	{
		sys_timer_usleep(100);
		_srr0 = load_vol(srr0);
	}

	printf("SRR0 as read from SPU: 0x%x\n", (u64)_srr0);

	store_vol(srr0, -1);

	sys_ppu_thread_t thr;
	thread_create(&thr, &signal_thread, 0, 1000, 1<<20, 0, "SIGNAL_HANDLER");

	_srr0 = -1;
	while (_srr0 == -1)
	{
		sys_timer_usleep(100);
		_srr0 = load_vol(srr0);
	}

	printf("SRR0 as read from PPU: 0x%llx\n", (u64)_srr0);

	int cause;
	int status;
	ENSURE_OK(sys_spu_thread_group_terminate(grp_id, 0));

	spu_printf_detach_group(grp_id);
	spu_printf_finalize();

	ENSURE_OK(sys_spu_thread_group_destroy(grp_id));

	return 0;
}
