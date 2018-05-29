#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_syscache.h>
#include <cell/sysmodule.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <functional>

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t memory_id;
sys_addr_t addr1;
sys_addr_t addr2;

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int32_t s32;

int main() {

	int result;

	sys_memory_t mem_id;

	printf("allocaddr\n");
	result = sys_mmapper_allocate_address(0x20000000, SYS_MEMORY_PAGE_SIZE_64K, 0x10000000, &addr1);
	if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
    }

	printf("allocmem\n");
	result = sys_mmapper_allocate_memory(0x100000, SYS_MEMORY_GRANULARITY_1M, &mem_id);
	if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
    }

	printf("mapmem\n");
	result = sys_mmapper_map_memory(u64(addr1) + (1<<16) /* Unaligned!!! */, mem_id, SYS_MEMORY_PROT_READ_WRITE);
	if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
    }
	
    printf("sample finished.\n");

    return 0;
}