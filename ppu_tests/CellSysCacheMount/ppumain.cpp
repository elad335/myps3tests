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
	int ret = cellSysCacheMount(&_cache);
	printf("cache path=%s, ret=0x%x\n", +_cache.getCachePath, ret);
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

	CacheMount(param);
	std::string strtemp(param.getCachePath);
	strtemp += "/temp.txt";

	{
		int fd = -1;
		ENSURE_OK(cellFsOpen(strtemp.c_str(), CELL_FS_O_CREAT, &fd, NULL, 0));
		// As part of the testcase: do not close file handle.
	}

	strcpy_trunc(param2.cacheId, cache_id, 14);
	//param2.cacheId[0] = '\0';

	CacheMount(param2);
	//ENSURE_OK(cellSysCacheClear());
	CacheMount(param);
	CacheMount(param2);

	std::string strtemp2(param.getCachePath);
	strtemp2 += "/temp.txt";

	{
		int fd = -1;
		printf("cellFsOpen: ret=0x%x\n", cellFsOpen(strtemp2.c_str(), 0, &fd, NULL, 0));
	}

	// Mount again
	CacheMount(param2);
	printf("sample finished.\n");

	return 0;
}