#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

#include "../../spu_header.h"

int main(void)
{
	spu_write_out_mbox(mfc_read_tag_mask());
	return 0;
}