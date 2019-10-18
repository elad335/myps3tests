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
	cellFunc(SysCacheMount, &_cache);
	printf("cache path=%s, id=%s\n", +_cache.getCachePath, +_cache.cacheId);
}

void strcpy_trunc(char* dst, const char* src, size_t max_count)
{
	if (max_count > 1) strncpy(dst, src, max_count - 1);
	dst[max_count - 1] = '\0';
}

#define cache_id ( "16-45-01-83-01" )

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );

	CellSysCacheParam param = {}, param2 = {};

	strcpy_trunc(param.cacheId, cache_id, 15);
	//param.cacheId[0] = '\0';
	strcpy_trunc(param2.cacheId, cache_id, 14);
	//param2.cacheId[0] = '\0';

	CacheMount(param);
	cellSysCacheClear();

	std::string strtemp(param.getCachePath);
	strtemp += "/temp.txt";

	//{
		int first_fd = -1;
		ENSURE_OK(cellFsOpen(strtemp.c_str(), CELL_FS_O_CREAT, &first_fd, NULL, 0));
		// As part of the testcase: do not close file handle.
	//}

	// Mount again the original id
	CacheMount(param);

	// Invalidate path
	strtemp = param.getCachePath;
	strtemp += "/temp.txt";

	{
		int fd = -1;
		cellFunc(FsOpen, strtemp.c_str(), 0, &fd, NULL, 0);

		if (g_ec == CELL_OK)
		{
			ENSURE_OK(cellFsClose(fd));
		}
	}

	// Mount with different id
	CacheMount(param2);

	// Mount with the original id
	CacheMount(param);

	// Invalidate path
	strtemp = param.getCachePath;
	strtemp += "/temp.txt";

	{
		int fd = -1;
		cellFunc(FsOpen, strtemp.c_str(), 0, &fd, NULL, 0);

		if (g_ec == CELL_OK)
		{
			ENSURE_OK(cellFsClose(fd));
		}
	}

	printf("Reading from the first file handle.\n");

	char buf[1];
	u64 nread = ~0ull;
	cellFunc(FsRead, first_fd, buf, 1, &nread);

	printf("sample finished.\n");

	return 0;
}