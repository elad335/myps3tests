
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ppu_thread.h> 
#include <sys/spu_thread.h>
#include <sys/raw_spu.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>
#include <sys/interrupt.h>
#include <sys/paths.h>
#include <sys/process.h>
#include <spu_printf.h>
#include <sysutil/sysutil_sysparam.h>  
#include <sys/timer.h>
#include <spu_printf.h>
#include "SPUThread.h"

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack	: 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];

static sys_raw_spu_t thr_id = ~0x0;
register u64 stack asm ("1");
static u32 ret = 0;

void threadEntry(u64)
{
	sys_ppu_thread_stack_t sp;
	sys_ppu_thread_get_stack_information(&sp);
	printf("intr thread: stack addr=%x current stack offset=0x%x\n", sp.pst_addr, (sp.pst_addr + sp.pst_size) - stack);
	u32 value; sys_raw_spu_read_puint_mb(thr_id, &value); fsync();
	sys_raw_spu_set_int_stat(thr_id, 2, SPU_INT2_STAT_MAILBOX_INT);
	sys_interrupt_thread_eoi();
}

int main(void)
{
	sys_ppu_thread_stack_t sp;
	sys_ppu_thread_get_stack_information(&sp);
	printf("main thread: stack addr=%x current stack offset=0x%x\n", sp.pst_addr, (sp.pst_addr + sp.pst_size) - stack);
	sys_timer_usleep(3000);

	sys_spu_initialize(6, 1); // 1 raw threads max

	static sys_ppu_thread_t ppuThreadId;

	/*E Create an interrupt PPU thread. */
	ret = sys_ppu_thread_create(&ppuThreadId,threadEntry,0, 1, 0x4000, SYS_PPU_THREAD_CREATE_INTERRUPT, "RawSPU Interrupt Thread0");
	if (ret != CELL_OK) {
		printf("sys_ppu_thread_create() failed: 0x%08x\n", ret);
		return -1;
	}

	static sys_spu_image_t img;

	ENSURE_OK(sys_spu_image_import(&img, (void*)_binary_test_spu_spu_out_start, SYS_SPU_IMAGE_DIRECT));

	ret = sys_raw_spu_create(&thr_id, NULL);
	if (ret != CELL_OK) {
		printf("sys_spu_image_import: %x\n", ret);
		return ret;
	}
	
	ret = sys_raw_spu_image_load(thr_id, &img);
	if (ret != CELL_OK) {
		printf("sys_spu_image_import: %x\n", ret);
		return ret;
	}

	static sys_interrupt_tag_t tag;
	sys_raw_spu_create_interrupt_tag(thr_id, 2, SYS_HW_THREAD_ANY, &tag);
	/**
	 *E Set the class 2 interrupt mask to handle the SPU Outbound Mailbox
	 *  interrupts.
	 */
	ENSURE_OK(sys_raw_spu_set_int_mask(thr_id, 2, SPU_INT2_STAT_MAILBOX_INT));

	static sys_interrupt_thread_handle_t ih;
	ENSURE_OK(sys_interrupt_thread_establish(&ih, tag, ppuThreadId, 0));


	sys_raw_spu_mmio_write(thr_id, SPU_RunCntl, 1); // invoke raw spu thread

	while ((sys_raw_spu_mmio_read(thr_id, SPU_Status) & 0x1) == 0)
		fsync(); // waits until the run requast has been completed

	while ((sys_raw_spu_mmio_read(thr_id, SPU_Status) & 0x1))
	{
		sys_timer_usleep(4000);
	} // break when spu has stopped, acts like std::thread.join()

	ENSURE_OK(sys_raw_spu_destroy(thr_id));

	return 0;
}

