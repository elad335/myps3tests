#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <string.h>
   // volatile char failedBuf[8192] __attribute__((aligned(8192))) = {0};
    //static volatile int srr = 0;
    static volatile int pc = 0;
static void intr()
{
   // pc++;
    __asm__ volatile ("ahi $81, $81 , 1");
	spu_writech(MFC_WrTagMask, 0x30);
	asm volatile ("rdch $82, MFC_RdTagMask");
    __asm__ volatile ("iretd");
}
 int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    register int reg __asm__ ("81");
    register int reg2 __asm__ ("82");
    reg = 0;
       memcpy(reinterpret_cast<void*>(0) ,reinterpret_cast<const void *>(&intr), 32);
    (void)arg2;
	(void)arg4;

    //char scratchBuf[8192] __attribute__((aligned(128))) = {0};
    //const uint32_t* start = (void*)0; 


    spu_printf("waiting for an error ,else stall\n");
    //spu_ack_events();
      spu_ienable();
    spu_writech(SPU_WrDec, 0x10000000);
    spu_writech(SPU_WrEventMask, 0x20);
	    spu_writech(MFC_WrTagMask, 0x20);
     asm ("l1:");
	asm volatile ("rchcnt $92, SPU_RdEventStat");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("ahi $125, $125, 12");
	asm volatile ("sfhi $125, $125, 12");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("nop $125");
	asm volatile ("nop $127");
	asm volatile ("brz $92, l1");

  //  for (int i = 0; i< 30; i++){pc++;}
 //  __asm__ volatile ("il $82, nojump");

// __asm__ volatile ("bi $82");
// __asm__ volatile ("nojump:");
		//__asm__ volatile ("syncc");
		//__asm__ volatile ("sync");
		//__asm__ volatile ("dsync");
    spu_idisable();
   
   spu_printf("srr = %x\n", spu_readch(SPU_RdSRR0));
    spu_printf("82 = %x\n", reg2 );
	spu_printf("tag mask = %x\n",spu_readch(MFC_RdTagMask));
   // spu_printf("done %x\n", intr);
          spu_printf("event count = %x\n",spu_readchcnt(SPU_RdEventStat));
           spu_printf("81 = %x\n",reg);
       if (spu_readchcnt(SPU_RdEventStat)) spu_printf("events = %x\n", spu_readch(SPU_RdEventStat));
    //spu_readch(SPU_RdEventStat);

   
        //__asm__ volatile ("stopd $2, $2, $2");
    sys_spu_thread_exit(0);
	return 0;
}

// result - interrupt fired, spe checks after every instruction if an interrupt condition is met and 