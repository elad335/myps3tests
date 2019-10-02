#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
   // volatile char failedBuf[8192] __attribute__((aligned(8192))) = {0};
	//static volatile int srr = 0;
	static volatile int pc = 0;
static void intr()
{
   // pc++;
	__asm__ volatile ("ahi $81, $81 , 1");
	__asm__ volatile ("sync");
	__asm__ volatile ("syncc");
	__asm__ volatile ("dsync");
	__asm__ volatile ("iret");
}
 int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	register int reg __asm__ ("81");
	register int reg2 __asm__ ("82");
	reg = 0;
	   memcpy(0 ,intr, 20);
	(void)arg2;
	(void)arg4;

	//char scratchBuf[8192] __attribute__((aligned(128))) = {0};
	//const uint32_t* start = (void*)0; 


	spu_printf("waiting for an error ,else stall\n");
	//spu_ack_events();
	  spu_ienable();
	spu_writech(SPU_WrDec, 0x10000000);
	spu_writech(SPU_WrEventMask, 0x20);
	 __asm__ ("l1:");
	 	__asm__ volatile ("rchcnt $92, SPU_RdEventStat");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		//__asm__ volatile ("dsync");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		//__asm__ volatile ("sync");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		//__asm__ volatile ("syncc");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("ahi $125, $125, 12");
		__asm__ volatile ("sfhi $125, $125, 12");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
		__asm__ volatile ("nop $125");
		__asm__ volatile ("nop $127");
	__asm__ volatile ("brz $92, l1");

  //  for (int i = 0; i< 30; i++){pc++;}
 //  __asm__ volatile ("il $82, nojump");

// __asm__ volatile ("bi $82");
// __asm__ volatile ("nojump:");
		__asm__ volatile ("syncc");
		__asm__ volatile ("sync");
		__asm__ volatile ("dsync");
	spu_idisable();
   
   spu_printf("done %x\n", spu_readch(SPU_RdSRR0));
	spu_printf("done %x\n", reg2 );
   // spu_printf("done %x\n", intr);
		  spu_printf("%x\n",spu_readchcnt(SPU_RdEventStat));
		   spu_printf("%x\n",reg);
	   if (spu_readchcnt(SPU_RdEventStat)) spu_printf("done %x\n", spu_readch(SPU_RdEventStat));
	//spu_readch(SPU_RdEventStat);

   
		//__asm__ volatile ("stopd $2, $2, $2");
	sys_spu_thread_exit(0);
	return 0;
}

// result - interrupt fired, spe checks after every instruction if an interrupt condition is met and 