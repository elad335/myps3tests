
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>

#include <sys/spu_thread_group.h>
#include <sys/spu_thread.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>

#include <spu_printf.h>
#include <sys/memory.h>

#include "../../ppu_tests/ppu_header.h"

/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];

int main(void)
{
	uint32_t ret;
	
	sys_spu_initialize(6, 5);

	sys_spu_thread_group_t grp_id;
	uint32_t prio = 100;
	sys_spu_thread_group_attribute_t grp_attr;
	
	grp_attr.name = NULL;
	grp_attr.nsize = 0;
	grp_attr.type = 0x20;
	ENSURE_OK(sys_memory_container_create(&grp_attr.option.ct, 1ull << 20));

	sys_spu_thread_group_attribute_name(grp_attr, "test spu grp");
	ret = sys_spu_thread_group_create(&grp_id, 1, prio, &grp_attr);
	if (ret != CELL_OK) {
		printf("spu_thread_group_create failed: %x\n", ret);
		//return ret;
	}

	ENSURE_OK(sys_spu_thread_group_destroy(grp_id));

	return 0;
}
