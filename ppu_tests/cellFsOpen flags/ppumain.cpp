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
	cellSysmoduleLoadModule( CELL_SYSMODULE_FS );

	int ret = 0;
	int fd = -1;
	const char* fpath = "/app_home/testdir/file.h";

	ret = cellFsMkdir("/app_home/testdir", CELL_FS_DEFAULT_CREATE_MODE_3);

	if (ret != CELL_OK && ret != EEXIST)
	{
		// Failed to create dir
		printf("Failure!\n");
		exit(-1);
	}

	int flags[3] = {CELL_FS_O_EXCL | CELL_FS_O_RDWR, CELL_FS_O_EXCL | CELL_FS_O_TRUNC | CELL_FS_O_RDWR, CELL_FS_O_EXCL | CELL_FS_O_APPEND};

	for (u32 it = 0; it < 3; it++)
	{
		cellFsUnlink(fpath); // Ensure file does not exist
		printf("cellFsOpen%d :0x%x\n", (it * 2) + 1, ret = cellFsOpen(fpath,flags[it], &fd, NULL, 0));

		if (ret != CELL_OK)
		{
			ret = cellFsOpen(fpath,CELL_FS_O_RDWR | CELL_FS_O_CREAT, &fd, NULL, 0);

			if (ret != CELL_OK)
			{
				// Failed to create file
				printf("Failure!\n");
				exit(-1);
			}

			cellFsClose(fd);

			printf("cellFsOpen%d :0x%x\n", (it * 2) + 2, ret = cellFsOpen(fpath,flags[it], &fd, NULL, 0));
			cellFsClose(fd);
			continue;
		}
		else
		{
			cellFsClose(fd);
		}
	}

	cellFsUnlink(fpath);
	cellFsRmdir("/app_home/testdir");
	printf("sample finished.\n");

	return 0;
}