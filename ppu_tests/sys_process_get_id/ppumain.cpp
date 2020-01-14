
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/process.h>
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
#include <sys/raw_spu.h>
#include "../ppu_header.h"

int main(void)
{
	u32 out_size = -1;
	u32 ids[8192] = {};
	sysCell(process_get_id, SYS_MUTEX_OBJECT, ids, -1u, &out_size);
	printf("sys_process_get_id(-1): out_size=0x%x\n", out_size);
	sysCell(process_get_id, SYS_MUTEX_OBJECT, ids, 0, &out_size);
	printf("sys_process_get_id(0): out_size=0x%x\n", out_size);

	return 0;
}

