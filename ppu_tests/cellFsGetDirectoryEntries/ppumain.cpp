#include <stdio.h>
#include <stdlib.h>

#include <cell/cell_fs.h>
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
// Allows for cache id to be 32 characters long (not including null term)
SYS_PROCESS_PARAM(1000, 0x10000)

void strcpy_trunc(char* dst, const char* src, size_t max_count)
{
	if (max_count > 1) strncpy(dst, src, max_count - 1);
	dst[max_count - 1] = '\0';
}

void CacheMount(CellSysCacheParam& _cache)
{
	char buf_id[CELL_SYSCACHE_ID_SIZE + 1];
	strcpy_trunc(buf_id, _cache.cacheId, sizeof(buf_id));
	cellFunc(SysCacheMount, &_cache);
	printf("cache path=%s, id=%s\n", _cache.getCachePath, buf_id);
}

#define cache_id  "16-45-01-83"

void printBytes(void* data, size_t size)
{
	for (u32 i = 0; i < size; i++)
	{
		printf("[%04x] %02X\n", i, static_cast<u8*>(data)[i]);
	}	
}

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );

	CellSysCacheParam param = {};

	strcpy_trunc(param.cacheId, cache_id, 15);

	CacheMount(param);
	cellSysCacheClear();

	std::string strtemp(param.getCachePath);
	strtemp += "/MyDir/";

	ENSURE_OK(cellFsMkdir(strtemp.c_str(), CELL_FS_DEFAULT_CREATE_MODE_3));

	s32 fd = -1;
	u64 nread = -1;
	CellFsDirectoryEntry entry[2];
	u32 count = -1;
	std::memset(&entry, 0xff, sizeof entry);
	ENSURE_OK(cellFsOpendir(strtemp.c_str(), &fd));

	ENSURE_OK(cellFsGetDirectoryEntries(fd, entry, sizeof entry, &count));
	printf("data_count = 0x%x\n", count);
	printBytes(entry, sizeof entry);
	std::memset(&entry, 0xff, sizeof entry);
	ENSURE_OK(cellFsGetDirectoryEntries(fd, entry, sizeof entry, &count));
	printf("data_count = 0x%x\n", count);
	printBytes(entry, sizeof entry);
	std::memset(&entry, 0xff, sizeof entry);
	ENSURE_OK(cellFsClosedir(fd));

	/*ENSURE_OK(cellFsOpendir(strtemp.c_str(), &fd));

	ENSURE_OK(cellFsReaddir(fd, &entry[0].entry_name, &nread));
	ENSURE_OK(cellFsReaddir(fd, &entry[0].entry_name, &nread));
	ENSURE_OK(cellFsReaddir(fd, &entry[0].entry_name, &nread));
	
	std::memset(&entry[0].entry_name, 0xff, sizeof (entry[0].entry_name));
	ENSURE_OK(cellFsReaddir(fd, &entry[0].entry_name, &nread));
	printf("nread = 0x%x\n", nread);
	printBytes(&entry[0].entry_name, sizeof (entry[0].entry_name));	
	ENSURE_OK(cellFsClosedir(fd));*/
	return 0;
}