
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

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;
#define true (1)
#define false (0)
#define bool u8

inline void sync() {asm volatile ("db16cyc;eieio;sync");};

/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];

static u8 g_interrupt_count = 0x0;
static sys_raw_spu_t thr_id = ~0x0;
static u32 ret = 0;

void ppu_interrupt(u64 unused)
{
	printf("INTERRUPT FIRED!!\n");

	//if (false)//if ((++g_interrupt_count) >= 2)
	{

		// Reading the interrupt maiblox only for the second interrupt or more
		{sync(); u32 unused_value; ret = sys_raw_spu_read_puint_mb(thr_id, &unused_value); sync();}

		if (ret != CELL_OK) 
		{
			printf("sys_raw_spu_read_puint_mb() failed: 0x%08x\n", ret);
			return sys_interrupt_thread_eoi();
		}
	}

	// Write into the inbound mailbox only when its empty
	if (((sys_raw_spu_mmio_read(thr_id, SPU_MBox_Status_offs) & 0xff00) >> 8) == 4)
	{
		sys_raw_spu_mmio_write(thr_id, SPU_In_MBox_offs, 0);
		sync();
	}

	// Not reading the interrupt mailbox intentionally
	//sys_raw_spu_set_int_stat(thr_id, 2, SPU_INT2_STAT_MAILBOX_INT);

	sys_interrupt_thread_eoi();
}

int main(void)
{
	ret = sys_spu_initialize(6, 1); // 1 raw threads max
	if (ret != CELL_OK) {
        printf("sys_spu_initialize failed: %x\n", ret);
        return ret;
	}

	sys_ppu_thread_t ppuThreadId;

	/*E Create an interrupt PPU thread. */
	ret = sys_ppu_thread_create(
				&ppuThreadId,
				ppu_interrupt,	/*E Pointer of the entry function */
				0,		/*E Argument of the entry function */
				1,
				0x4000,
				SYS_PPU_THREAD_CREATE_INTERRUPT,	/*E Interrupt PPU Thread */
				"RawSPU Interrupt Thread0");
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

    ret = sys_raw_spu_create(&thr_id, NULL);
	if (ret != CELL_OK) {
        printf("sys_spu_image_import: %x\n", ret);
        return ret;
    }

    ret = spu_printf_initialize(1000, NULL);
    if (ret != CELL_OK) {
        printf("spu_printf_initialize failed %x\n", ret);
        exit(-1);
    }
	
	ret = sys_raw_spu_image_load(thr_id, &img);
	if (ret != CELL_OK) {
        printf("sys_spu_image_import: %x\n", ret);
        return ret;
    }

	sys_interrupt_tag_t tag;
	sys_raw_spu_create_interrupt_tag(thr_id, 2, SYS_HW_THREAD_ANY, &tag);
	/**
	 *E Set the class 2 interrupt mask to handle the SPU Outbound Mailbox
	 *  interrupts.
	 */
	ret = sys_raw_spu_set_int_mask(
				thr_id,
				2, //SPU_INTR_CLASS_2,
				SPU_INT2_STAT_MAILBOX_INT);
	if (ret != CELL_OK) {
		printf("sys_raw_spu_set_int_mask() failed: 0x%08x\n", ret);
		return -4;
	}

	sys_interrupt_thread_handle_t ih;
	ret = sys_interrupt_thread_establish(
				&ih,
				tag,
				ppuThreadId,
				0);
	if (ret != CELL_OK) {
		printf("sys_interrupt_thread_establish() failed: 0x%08x\n", ret);
		return ret;
	}


	sys_raw_spu_mmio_write(thr_id, SPU_RunCntl, 1); // invoke raw spu thread

	while ((sys_raw_spu_mmio_read(thr_id, SPU_Status) & 0x1) == 0)
		sync(); // waits until the run requast has been completed

	printf("raw thread is running\n");

	while ((sys_raw_spu_mmio_read(thr_id, SPU_Status) & 0x1))
	{
		sync();
	} // break when spu has stopped, acts like std::thread.join()

	printf("raw thread has stopped [num of interrupts: %x]\n", g_interrupt_count);

	printf("the spu status register's value is %x\n", sys_raw_spu_mmio_read(thr_id, SPU_Status));

    ret = sys_raw_spu_destroy(thr_id);
    if (ret != CELL_OK) {
        printf("sys_raw_spu_destroy: %x\n", ret);
        return ret;
    }

    spu_printf_finalize();

	return 0;
}

