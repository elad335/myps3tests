#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	   
	(void)arg1;
	(void)arg2;
	(void)arg4;

	//char scratchBuf[8192] __attribute__((aligned(128))) = {0};
	//const uint32_t* start = (void*)0; 
		spu_printf("waiting..\n");
  volatile char failedBuf[8192] __attribute__((aligned(128))) = {0};
	spu_writech(SPU_WrOutMbox, 9);
		spu_writech(SPU_WrOutMbox, 9);
	spu_printf("didnt stall on threaded spu\n");

	sys_spu_thread_exit(0);
	return 0;
}