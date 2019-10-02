
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
#include <cell/atomic.h>
#include "SPUThread.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

inline void sync() {asm volatile ("db16cyc;eieio;sync");};

/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];

static u32 g_interrupt_count = 0;
static sys_raw_spu_t thr_id = -1u;
static u32 ret = 0;

#define eoi() sys_interrupt_thread_eoi()

void ppuInterrupt(u64)
{
	g_interrupt_count++; sync();

	u64 stat;
	sys_raw_spu_get_int_stat(thr_id, 2, &stat);
	printf("Interupt fired: intr status=0x%x\n", stat); sync();

	{u32 unused_value; ret = sys_raw_spu_read_puint_mb(thr_id, &unused_value);}

	if (ret != CELL_OK) 
	{
		printf("sys_raw_spu_read_puint_mb() failed: 0x%08x\n", ret);
		eoi();
	}

	if (g_interrupt_count >= 10) // Avoid TTY spam
	{
		sys_raw_spu_set_int_stat(thr_id, 2, SPU_INT2_STAT_MAILBOX_INT);
	}

	//sys_raw_spu_set_int_stat(thr_id, 2, SPU_INT2_STAT_MAILBOX_INT);
	sync();
	eoi();
}

int main(void)
{
	ret = sys_spu_initialize(6, 1);
	if (ret != CELL_OK) {
		printf("sys_spu_initialize failed: %x\n", ret);
		return ret;
	}

	sys_ppu_thread_t ppuId;

	/*E Create an interrupt PPU thread. */
	ret = sys_ppu_thread_create(&ppuId, ppuInterrupt, 0, 1, 0x8000, SYS_PPU_THREAD_CREATE_INTERRUPT, "RawSPU Interrupt Thread0");
	if (ret != CELL_OK) {
		printf("sys_ppu_thread_create() failed: 0x%08x\n", ret);
		return -1;
	}

	sys_spu_image_t img;

	ret = sys_spu_image_import(&img, (void*)_binary_test_spu_spu_out_start, SYS_SPU_IMAGE_DIRECT);
	if (ret != CELL_OK) {
		printf("sys_spu_image_import: %x\n", ret);
		return ret;
	}

	spu_printf_initialize(1000, NULL);

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

	sys_interrupt_tag_t tag;
	sys_raw_spu_create_interrupt_tag(thr_id, 2, SYS_HW_THREAD_ANY, &tag);

	u64 mask;
	sys_raw_spu_get_int_mask(thr_id, 2, &mask);
	printf("Initial interrupt mask class 2=0x%x\n", mask);
	sys_raw_spu_get_int_mask(thr_id, 0, &mask);
	printf("Initial interrupt mask class 0=0x%x\n", mask);

	ret = sys_raw_spu_set_int_mask(thr_id, 2, SPU_INT2_STAT_MAILBOX_INT);
	if (ret != CELL_OK) {
		printf("sys_raw_spu_set_int_mask() failed: 0x%08x\n", ret);
		return -4;
	}

	sys_interrupt_thread_handle_t ih;
	ret = sys_interrupt_thread_establish(&ih, tag, ppuId, 0);
	if (ret != CELL_OK) {
		printf("sys_interrupt_thread_establish() failed: 0x%08x\n", ret);
		return ret;
	}

	sys_raw_spu_mmio_write(thr_id, SPU_RunCntl, 1); // invoke raw spu thread

	while ((sys_raw_spu_mmio_read(thr_id, SPU_Status) & 0x1) == 0)
		sync(); // waits until the run request has been completed

	printf("raw thread is running\n");

	// Join raw spu thread
	while (!(sys_raw_spu_mmio_read(thr_id, SPU_Status) & SPU_STATUS_STOPPED_BY_STOP))
	{
		sys_timer_usleep(400);
		sync();
	}

	sys_timer_sleep(2);
	printf("raw thread has stopped [interrupts count: %d]\n", g_interrupt_count);

	sys_interrupt_thread_disestablish(ih);
	sys_interrupt_tag_destroy(tag);

	ret = sys_raw_spu_destroy(thr_id);
	if (ret != CELL_OK) {
		printf("sys_raw_spu_destroy: %x\n", ret);
		return ret;
	}

	return 0;
}

