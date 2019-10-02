#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_mfcio_gcc.h>
#include <spu_internals.h>
#include <stdint.h>

#include "../spu_header.h"

int main() {

	uint32_t addr = spu_readch(SPU_RdSigNotify1); // Get the mfc address sent by the ppu thread
	u32 arr[1024] __attribute__((aligned(256))) = {0x80000010, addr, 0x00000010, addr}; // One stalling element
	u32 arr2[1024] __attribute__((aligned(256))) = {0};

	spu_writech(MFC_EAL, int_cast(&arr[0]));
	spu_writech(MFC_LSA , int_cast(&arr2[0]));
	spu_writech(MFC_TagID , 0);
	spu_writech(MFC_Size , 0x10);
	spu_writech(MFC_Cmd, 0x44); // GETL 
	spu_readch(MFC_RdListStallStat); // Wait for the list to stall

	spu_writech(MFC_EAL, addr);
	spu_writech(MFC_LSA , int_cast(&arr2[0]));
	spu_writech(MFC_TagID , 0);
	spu_writech(MFC_Size , 1);
	spu_writech(MFC_Cmd, 0x41); // GETB

	spu_printf("Waiting for the command to complete");

	while (spu_readchcnt(MFC_Cmd) < 15) fsync(); // Wait for one command from the two pushed ones to complete

	spu_printf("Finished, didnt stall");
	return 0;
}