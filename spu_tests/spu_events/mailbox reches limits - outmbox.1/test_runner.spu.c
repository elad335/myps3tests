#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	   
	//while(1)spu_writech(SPU_WrOutMbox, 9);
	//char scratchBuf[8192] __attribute__((aligned(128))) = {0};
	//const uint32_t* start = (void*)0; 
  //volatile char failedBuf[8192] __attribute__((aligned(128))) = {0};

	spu_printf("waiting..\n");
	//while (1){}
	spu_writech(SPU_WrOutMbox, 0x1234);
	__asm__ ("stop 0x102");
	//sys_spu_thread_exit(0x1234);
	return 0;
}

// results: if the output mailbox is full while the spu is runnunfg, CELL_ESTAT returned, if its stoppes, CELL_OK returned, and the same call can excuate infinitelly and the the exit status wont xchange , hence its static and can be read as much as we want