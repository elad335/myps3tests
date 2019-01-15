#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>
#include <string.h>

#include "../spu_header.h"

static mfc_list_element_t dma_lists[2][2];
static lock_line_t s_data;

int main(u64 addr) {

    mfc_fence();

    // Fill the first elements with usual transfers
    dma_lists[0][0].size = sizeof(s_data);
    dma_lists[0][0].eal = addr;
    dma_lists[0][0].notify = 1;

    dma_lists[0][1].size = 0;
    dma_lists[0][1].eal = addr;
    dma_lists[0][1].notify = 0;

    dma_lists[1][0].size = sizeof(s_data);
    dma_lists[1][0].eal = addr;
    dma_lists[1][0].notify = 1;

    dma_lists[1][1].size = 0;
    dma_lists[1][1].eal = addr;
    dma_lists[1][1].notify = 0;

    mfc_getl(&s_data, 0, &dma_lists[0], 0x10, 0, 0,0);
    mfence();
    mfc_read_list_stall_status(); // Wait until stalled
    mfc_getl(&s_data, 0, &dma_lists[1], 0x10, 0, 0,0);
    mfence();

    mfc_read_list_stall_status(); // Wait until stalled
    mfc_write_list_stall_ack(0);
    mfence();

    while (mfc_stat_cmd_queue() < 16)
    {
        mfence();
    }

    spu_printf("sample finished.\n");
    mfc_fence();
    sys_spu_thread_exit(0);
	return 0;
}