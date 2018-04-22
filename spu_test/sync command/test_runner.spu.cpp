#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <cstring>

inline void sync() {asm volatile ("lnop;sync;syncc;dsync");};
//inline void print(const std::string str) {str.append("\n"); spu_printf(str);};

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    while(spu_readchcnt(SPU_RdSigNotify1) == 0){}
    uint32_t addr = spu_readch(SPU_RdSigNotify1);
    spu_printf("ok\n");
    char arr[8192] __attribute__((aligned(128))) = {0};
    unsigned int list[256] __attribute__((aligned(128))) = {0x80000100, addr, 0x80000100, addr}; // Stall and notify type list entry and an empty one
    char* ptr = &arr[0];
    unsigned char* ptr2 = (unsigned char*)(void*)(uintptr_t)&list[0]; // the compiler is strict ha

    spu_writech(MFC_EAH, 0);
    spu_writech(MFC_EAL, (uint32_t)ptr2);
    spu_writech(MFC_LSA , (uint32_t)ptr);
    spu_writech(MFC_TagID, 31);
    spu_writech(MFC_Size, 0x10); // Two list entries
    spu_writech(MFC_Cmd, 0x24); // Put list command
    sync();
    spu_printf("ok\n");
    while (spu_readchcnt(MFC_Cmd) == 16) {sync();}
    sync();
    spu_printf("ok\n");

    while (spu_readchcnt(MFC_RdListStallStat) == 0) {sync();} // Ensure the list has stalled
    spu_printf("ok\n");

    spu_writech(MFC_TagID, 1);
    spu_writech(MFC_Cmd, 0xCC); // SYNC command
    sync();

    while (spu_readchcnt(MFC_Cmd) >= 15){sync();}
    sync();
    spu_printf("ok");

    spu_writech(MFC_WrTagMask, 0x2); // SYNC command's tag
    spu_writech(MFC_WrTagUpdate, 2); // Wait until SYNC command has finished (according to the tag status)
    sync(); sync(); sync(); sync();
    spu_printf("the tag status is : %x \n", spu_readch(MFC_RdTagStat)); // result: stalled, shows the sync cmd acts like a full barrier

    sys_spu_thread_exit(0);
	return 0;
}