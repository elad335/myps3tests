#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>
#include <string.h>

#include "../spu_header.h"

static mfc_list_element_t dma_lists[2];
static lock_line_t s_data;

int main(u64 addr)
{
	mfcsync();

	// Fill the first elements with usual transfers
	dma_lists[0].size = 0;
	dma_lists[0].eal = 0;
	dma_lists[0].notify = 1;

	dma_lists[1].size = 0;
	dma_lists[1].eal = 0;
	dma_lists[1].notify = 0;

	for (u32 i = 0; i < 2; i++)
	{
		if (i == 0)
		{
			mfc_write_tag_mask(-1);
			mfc_write_tag_update(2);
			while (mfc_stat_tag_status() == 0);
			fsync();
		}

		mfc_getl(&s_data, 0, &dma_lists[0], 0x10, 0, 0,0);
		mfc_read_list_stall_status(); // Wait until stalled

		mfc_write_tag_mask(-1);
		mfc_write_tag_update(MFC_TAG_UPDATE_ALL);
	
		bool result = false;
	
		// Wait for condition to occur (with timeout)
		for (u32 j = 0; !result && j < 50000; j++)
		{
			fsync();
			result = (i == 0 ? mfc_stat_tag_status() : mfc_stat_tag_update()) == 0;
		}
	
		mfc_write_list_stall_ack(0);
	
		while (mfc_stat_cmd_queue() < 16)
		{
			fsync();
		}

		mfc_read_tag_status();
		spu_printf("Tag %s operation result: %d\n", i == 0 ? "Status" : "Update", +result);
	}

	sys_spu_thread_exit(0);
	return 0;
}