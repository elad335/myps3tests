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
#include <cell/gcm.h>
#include <memory>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

static sys_memory_t mem_id; 
static sys_addr_t addr1, addr2;

int main() 
{

	sys_memory_allocate(0x18700000, 0x400, &addr1);
	asm volatile ("twi 0x10, 3, 0");

	sys_mmapper_allocate_address(0x10000000, 0x40f, 0x10000000, &addr2);
	printf("addr1=0x%x, addr2=0x%x\n", addr1, addr2);

    printf("sample finished.\n");

    return 0;
}