
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/sys_time.h> 
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
#include <stdint.h>
#include <sys/sys_time.h>
#include <cell/atomic.h>
#include <ppu_intrinsics.h>

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

#include "../ppu_header.h"

inline void sync() {asm volatile ("db16cyc;eieio;sync");};

/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];

static inline u64 _mftb()
{
	u64 _ret;
	do
	{
		_ret = __mftb();
	} while (_ret == 0);

	return _ret;
}

void threadEntry(u64)
{
	u64 before, after;
	before = _mftb();
	sys_timer_sleep(10);
	after = _mftb();
	printf("before=0x%llx, after=0x%llx, passed=0%llx\n", after - before);
}

static volatile u32 spu_dec_status[32] __attribute__((aligned(128))) = {-1, -1, -1, 0}; // First : before, after, passed, signal (rest padding)

int main(void)
{
	sys_spu_initialize(6, 1); // 1 raw threads max

	int ret;
    static sys_spu_image_t img;
	static sys_raw_spu_t thr_id;

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
	
	ret = sys_raw_spu_image_load(thr_id, &img);
	if (ret != CELL_OK) {
        printf("sys_spu_image_import: %x\n", ret);
        return ret;
    }

	{
		const u64 freq = sys_time_get_timebase_frequency();
		printf("frequancy:%d\n", freq);
	}

	//static sys_ppu_thread_t ppuThreadId;

	/*E Create an interrupt PPU thread. */
	//ret = sys_ppu_thread_create(&ppuThreadId,threadEntry,0, 1, 0x4000, 1, "RawSPU Interrupt Thread0");
	//if (ret != CELL_OK) {
	//	printf("sys_ppu_thread_create() failed: 0x%08x\n", ret);
	//	return -1;
	//}

	sys_raw_spu_mmio_write(thr_id, SPU_In_MBox, int_cast(&spu_dec_status[0]));
	sync();

	sys_raw_spu_mmio_write(thr_id, SPU_RunCntl, 1); // invoke raw spu thread

	while (spu_dec_status[3] == 0){} // waits until the run requast has been completed

	{
		u64 before = _mftb();
		sys_timer_sleep(10);
		u64 after = _mftb();
		cellAtomicStore32(const_cast<u32*>(&spu_dec_status[3]), -1);
		asm volatile ("sync");
		printf("PPU: before=0x%llx, after=0x%llx, passed=%lld\n", before, after, after - before);
	}

	while ((sys_raw_spu_mmio_read(thr_id, SPU_Status) & 0x1))
	{
		sys_timer_usleep(4000);
	} // break when spu has stopped, acts like std::thread.join()

	printf("SPU: before=0x%x, after=0x%x, passed=%u\n", spu_dec_status[0], spu_dec_status[1], spu_dec_status[2]);

    ret = sys_raw_spu_destroy(thr_id);
    if (ret != CELL_OK) {
        printf("sys_raw_spu_destroy: %x\n", ret);
        return ret;
    }

	return 0;
}

