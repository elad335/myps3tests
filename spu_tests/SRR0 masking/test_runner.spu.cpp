#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>
#include <string.h>
#include <cell/atomic.h>
#include <cell/dma.h>

#include "../spu_header.h"

int main(u64 addr) {

	spu_write_srr0(~0u);
	fsync(); // syncc must be issued here
	cellDmaPutUint64(spu_read_srr0(), addr, 0, 0, 0);

	// This is not funny! (puts the thread on STOP state directly)
	//asm volatile ("stopd");

	while (true)
	{
	}
	return 0;
}