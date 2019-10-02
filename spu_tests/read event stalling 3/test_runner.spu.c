#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
   // volatile char failedBuf[8192] __attribute__((aligned(8192))) = {0};
	//static volatile int srr = 0;
   // static volatile int pc = 0;
static void intr()
{
   // pc++;
	__asm__ volatile ("ai $81, $81 , 1");
	 // __asm__ volatile ("rdch $82, $SPU_RdSRR0");
	// __asm__ volatile ("ai $82, $82,4");
	 //  __asm__ volatile ("wrch $SPU_WrSRR0, $82");
   // si_syncc();
	// si_sync();
	 // si_dsync();
	__asm__ volatile ("iret");
}
 int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	register int reg __asm__ ("81");
	register int reg2 __asm__ ("82");
	register int reg3 __asm__ ("83");
	reg = 0;
	   memcpy(0 ,intr, 8);
	(void)arg2;
	(void)arg4;

	//char scratchBuf[8192] __attribute__((aligned(128))) = {0};
	//const uint32_t* start = (void*)0; 


	spu_printf("waiting for an error ,else stall\n");
	spu_ack_events();
	  //spu_ienable();
	spu_writech(SPU_WrDec, 80000000);
	spu_writech(SPU_WrEventMask, 0x20);
   //  while((int)spu_readch(SPU_RdDec) >= 0/*spu_readchcnt(SPU_RdEventStat) == 0*/){}

   spu_readch(SPU_RdEventStat);
	   spu_writech(SPU_WrEventAck, 0x0);
		   spu_writech(SPU_WrEventAck, ~0x0);
		//spu_ienable();
  //  for (int i = 0; i< 30; i++){pc++;}
 //  __asm__ volatile ("il $82, nojump");

// __asm__ volatile ("bi $82");
// __asm__ volatile ("nojump:");
	spu_idisable();
   
   spu_printf("done %x\n", spu_readch(SPU_RdSRR0));
	spu_printf("done %x\n", reg2 );
   // spu_printf("done %x\n", intr);
		  spu_printf("%x\n",spu_readch(SPU_RdEventStat));
		   spu_printf("%x\n",reg);
	   //spu_printf("done %x\n", spu_readch(SPU_RdEventStat));
	   spu_printf("done %x\n", reg3 );
	//spu_readch(SPU_RdEventStat);

   
		//__asm__ volatile ("stopd $2, $2, $2");
	sys_spu_thread_exit(0);
	return 0;
}

// result - interrupt fired, spe checks after every instruction if an interrupt condition is met and 