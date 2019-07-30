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

#include "../ppu_header.h"

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
	const char* dir_name = "BLES11VM";
	memcpy(param.cacheId, dir_name, 9);

	cellSysCacheMount(&param);

	printf("cellSysCacheMount: ret=0x%x, cache_path=%s\n", ret, param.getCachePath);

	cellSysCacheMount(&param);

	printf("cellSysCacheMount second:ret=0x%x, cache_path=%s\n", ret, param.getCachePath);

	int fd;
	std::string fpath(&param.getCachePath[0]);
	fpath += "/file.txt";
	cellFsOpen(fpath.c_str(), CELL_FS_O_CREAT, &fd, NULL, 0);

	printf("cellFsOpen: ret=0x%x\n", ret);
	cellFsClose(fd);

	cellSysCacheClear();

	cellFsOpen(fpath.c_str(), CELL_FS_O_RDONLY, &fd, NULL, 0);

	printf("cellFsOpen RO after clear: ret=0x%x\n", ret);
	cellFsClose(fd);

	cellFsOpen(fpath.c_str(), CELL_FS_O_CREAT, &fd, NULL, 0);

	printf("cellFsOpen with file creation after clear: ret=0x%x\n", ret);
	cellFsClose(fd);

    printf("sample finished.\n");

    return 0;
}