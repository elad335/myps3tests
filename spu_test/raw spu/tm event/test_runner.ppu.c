
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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


/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];

int main(void)
{
	int ret;
    
	ret = sys_spu_initialize(6, 2); // 2 raw threads max
	if (ret != CELL_OK) {
        printf("sys_spu_initialize failed: %x\n", ret);
        return ret;
	}

    sys_spu_image_t img;

    ret = sys_spu_image_import(&img, (void*)_binary_test_spu_spu_out_start, SYS_SPU_IMAGE_DIRECT);
    if (ret != CELL_OK) {
        printf("sys_spu_image_import: %x\n", ret);
        return ret;
    }

    sys_raw_spu_t thr_id;
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

	//sys_raw_spu_mmio_write(thr_id, SPU_NPC, 0);
	//asm volatile("eieio");
	sys_raw_spu_mmio_write(thr_id, SPU_RunCntl, 1); // invoke raw spu thread

	while ((sys_raw_spu_mmio_read(thr_id, SPU_Status) & 0x1) == 0) 
	{
		asm volatile ("eieio"); // waits until the run requast has been completed

	}

	printf("raw thread is running\n");
	
	while (sys_raw_spu_mmio_read(thr_id, SPU_Status) & 0x1)
	{
		asm volatile ("eieio");
		if (sys_raw_spu_mmio_read(thr_id, SPU_MBox_Status) & 0xff)
		{
			printf("spu decrementer's value is %x:\n written value to channel 69 is 0x7c00\n ",sys_raw_spu_mmio_read(thr_id, SPU_Out_MBox));
		}
		if (sys_raw_spu_mmio_read(thr_id, SPU_Status) & 0x8)
		{
			printf("spu is waiting on a blocked channel, destroying thread.\n current status is %x\n", sys_raw_spu_mmio_read(thr_id, SPU_Status));
			break;
		}
	} // break when spu has stopped, acts like std::thread.join()
	/*asm volatile ("eieio");
		sys_raw_spu_mmio_write(thr_id, SPU_RunCntl, 1);
		asm volatile ("eieio");*/
	///sys_raw_spu_mmio_write(thr_id, SPU_RunCntl, 0); // invoke raw spu thread

	//while ((sys_raw_spu_mmio_read(thr_id, SPU_Status) & 0x1) )
	///{
		///asm volatile ("eieio"); // waits until the run requast has been completed

	///}
	/*sys_raw_spu_mmio_write(thr_id, SPU_RunCntl, 1); // invoke raw spu thread

	while ((sys_raw_spu_mmio_read(thr_id, SPU_Status) & 0x1) == 0)
	{
		asm volatile ("eieio"); // waits until the run requast has been completed

	}

	sys_raw_spu_mmio_write(thr_id, SPU_RunCntl, 0); // invoke raw spu thread

	while ((sys_raw_spu_mmio_read(thr_id, SPU_Status) & 0x1))
	{
		asm volatile ("eieio"); // waits until the run requast has been completed

	}
	printf("raw thread has stopped\n");*/


	uint32_t current = sys_raw_spu_mmio_read(thr_id, SPU_RunCntl);
	printf("the current value of SPU_RunCntl is %x\n",sys_raw_spu_mmio_read(thr_id, SPU_RunCntl));
	printf("also the spu status register's value is %x\n", sys_raw_spu_mmio_read(thr_id, SPU_Status));
    ret = sys_raw_spu_destroy(thr_id);
    if (ret != CELL_OK) {
        printf("sys_raw_spu_destroy: %x\n", ret);
        return ret;
    }

    spu_printf_finalize();

	return 0;
}
