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
#include <cell/cell_fs.h>
#include <memory>
#include <iostream>
#include <string>
#include <list>
#include <map>

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define int_cast(addr) reinterpret_cast<uintptr_t>(addr)
#define ptr_cast(intnum) reinterpret_cast<void*>(intnum)

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;

sys_addr_t addr1;
sys_addr_t addr2;
sys_event_queue_t queue_id;

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );

	register int ret asm ("3");

	CellSysCacheParam param;
	const char* dir_name = "BLESSSSSSSSSSSSSSSSSSSLLVMMMMMMMMMMMMMMMMMMMMMMMMMMMM";
	memcpy(param.cacheId, dir_name, 32);

	cellSysCacheMount(&param);

	printf("cellSysCacheMount: ret=0x%x, OG cahceID = %s, cache_path=%s\n", ret, param.cacheId, param.getCachePath);

    printf("sample finished.\n");

    return 0;
}