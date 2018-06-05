
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/spu_thread_group.h>
#include <sys/spu_thread.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>
#include <sys/memory.h> 

typedef uint32_t u32;
sys_memory_info_t mem_info;

int main(void) {

	register u32 ret asm ("3");
    
	ret = sys_spu_initialize(6, 0);
	if (ret != CELL_OK) {
        printf("sys_spu_initialize failed: %d\n", ret);
        return ret;
	}

    sys_spu_thread_group_t grp_id;
    u32 prio = 100;
    sys_spu_thread_group_attribute_t grp_attr;
    
    sys_spu_thread_group_attribute_initialize(grp_attr);
    sys_spu_thread_group_attribute_name(grp_attr, "test spu grp");

    for (u32 i = 1; i <= 6; i++, prio++) {

        printf("number of spus: 0x%x\n", i);
        sys_memory_get_user_memory_size(&mem_info);
        printf("available user memory before group creation: 0x%x\n", mem_info.available_user_memory);

        ret = sys_spu_thread_group_create(&grp_id, i, prio, &grp_attr);
        if (ret != CELL_OK) {
            printf("spu_thread_group_create failed: %x\n", ret);
            return ret;
        }

        sys_memory_get_user_memory_size(&mem_info);
        printf("available user memory after group creation: 0x%x\n", mem_info.available_user_memory);

        ret = sys_spu_thread_group_destroy(grp_id);
        if (ret != CELL_OK) {
            printf("sys_spu_thread_group_destroy: %x\n", ret);
            return ret;
        }
    }

	return 0;
}
