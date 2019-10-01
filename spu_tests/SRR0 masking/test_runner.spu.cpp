#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>
#include <string.h>

#include "../spu_header.h"

int main(u64 addr) {

    spu_write_srr0(~0u);
    fsync(); // syncc must be issued here
    spu_printf("Value = 0x%x", spu_read_srr0());
	return 0;
}