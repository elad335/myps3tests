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

void CacheMount(CellSysCacheParam& _cache)
{
	char buf_id[CELL_SYSCACHE_ID_SIZE + 1] = {};
	strncpy(buf_id, _cache.cacheId, sizeof(buf_id));
	cellFunc(SysCacheMount, &_cache);
	printf("cache path=%s, id=%s\n", _cache.getCachePath, buf_id);
}

u64 FileSeek(u32 fd, s64 pos, s32 set = 0)
{
	u64 ret = 0;
	ENSURE_OK(cellFsLseek(fd, pos, set, &ret));
	return ret;
}

int main() {
	//return 0;
	cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );
	cellSysmoduleLoadModule( CELL_SYSMODULE_FS );
	cellSysmoduleLoadModule( CELL_SYSMODULE_RESC );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	CellSysCacheParam param = {};

	CacheMount(param);
	cellSysCacheClear();

	std::string strtemp(param.getCachePath);
	strtemp += "/temp.txt";
	const char* sdata_file_path = "/app_home/data.sdata";

	int temp_fd = open_file(strtemp.c_str(), CELL_FS_O_CREAT | CELL_FS_O_RDWR | CELL_FS_O_TRUNC);
	//ENSURE_OK(cellFsClose(temp_fd));

	int fds[10];
	memset(fds, -1, sizeof(fds));

	for (u32 i = 0; i < 10; i++)
	{
		fds[i] = open_file(strtemp.c_str(), CELL_FS_O_RDONLY);
		printf("fds[%d]: %d\n", i, fds[i]);
	}

	for (u32 i = 0; i < 10; i++)
	{
		printf("iteration variant 1: %d\n", i);

		for (u32 j = i; j != -1; j--)
		{
			const u32 pos = j % 2 == 0 ? j / 2 : 9 - j / 2; 
			ENSURE_OK(cellFsClose(fds[pos]));
		}

		for (u32 j = i; j != -1; j--)
		{
			const u32 pos = j % 2 == 0 ? j / 2 : 9 - j / 2; 
			fds[pos] = open_file(strtemp.c_str(), CELL_FS_O_RDONLY);
			printf("fds[%d]: %d\n", pos, fds[pos]);
		}
	}

	for (u32 i = 0; i < 10; i++)
	{
		printf("iteration varaint 2: %d\n", i);

		for (u32 j = i; j != -1; j--)
		{
			const u32 pos = j % 2 == 1 ? j / 2 : 9 - j / 2; 
			ENSURE_OK(cellFsClose(fds[pos]));
		}

		for (u32 j = i; j != -1; j--)
		{
			const u32 pos = j % 2 == 1 ? j / 2 : 9 - j / 2; 
			fds[pos] = open_file(strtemp.c_str(), CELL_FS_O_RDONLY);
			printf("fds[%d]: %d\n", pos, fds[pos]);
		}
	}

	return 0;
}