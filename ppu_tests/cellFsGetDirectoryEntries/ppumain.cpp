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

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_FS );

	CellSysCacheParam param = {};

	// Actually empty id to get it as empty dir
	strcpy_trunc(param.cacheId, cache_id, 0);

	CacheMount(param);

	std::string strtemp(param.getCachePath);
	strtemp += "/MyDir/";

	ENSURE_OK(cellFsMkdir(strtemp.c_str(), CELL_FS_DEFAULT_CREATE_MODE_1));

	s32 fd = -1;

	ENSURE_OK(cellFsOpen((strtemp + "txt.txt").c_str(), CELL_FS_O_CREAT, &fd, NULL, 0));
	ENSURE_OK(cellFsClose(fd));

	u64 nread = -1;
	CellFsDirectoryEntry entry[2]; // Two entries
	u32 count = -1;
	ENSURE_OK(cellFsOpendir(strtemp.c_str(), &fd));

	do
	{
		reset_obj(entry, 0xff);
		ENSURE_OK(cellFsGetDirectoryEntries(fd, entry, sizeof entry, &count));
		printf("data_count = 0x%x\n", count);

		for (u32 i = 0; i < count; i++)
		{
			printf("entry[%d]:\n", i);

			if (i < count)
			{
				printf("d_name = \"%s\"\n", entry[i].entry_name.d_name);
				printf("uid=%lld, gid=%lld\n", entry[i].attribute.st_uid, entry[i].attribute.st_gid);
			}

			print_obj(entry[i]);
		}
	}
	while (count);

	ENSURE_OK(cellFsClosedir(fd));

	for (u32 i = 0; i < 3; i++)
	{
		ENSURE_OK(cellFsOpendir(strtemp.c_str(), &fd));

		nread = 0;

		while (true)
		{
			reset_obj(entry[0].entry_name, 0xff - i);
			ENSURE_OK(cellFsReaddir(fd, &entry[0].entry_name, &nread));
			printf("Count(%i): nread = 0x%x\n", i, nread);
			print_obj(entry[0].entry_name, "entry");

			if (nread == 0)
			{
				reset_obj(entry[0].entry_name, 0xff - i);
				ENSURE_OK(cellFsReaddir(fd, &entry[0].entry_name, &nread));
				printf("Count(%i): nread = 0x%x\n", i, nread);
				print_obj(entry[0].entry_name, "entry");
				break;
			}
		}

		ENSURE_OK(cellFsClosedir(fd));
	}

	for (u32 i = 0; i < 3; i++)
	{
		ENSURE_OK(cellFsOpendir("/dev_hdd0/game/", &fd));

		nread = 0;

		while (true)
		{
			reset_obj(entry[0].entry_name, 0xff - i);
			ENSURE_OK(cellFsReaddir(fd, &entry[0].entry_name, &nread));
			printf("Count(%i): nread = 0x%x\n", i, nread);
			print_obj(entry[0].entry_name, "entry");

			if (nread == 0)
			{
				reset_obj(entry[0].entry_name, 0xff - i);
				ENSURE_OK(cellFsReaddir(fd, &entry[0].entry_name, &nread));
				printf("Count(%i): nread = 0x%x\n", i, nread);
				print_obj(entry[0].entry_name, "entry");
				break;
			}
		}

		ENSURE_OK(cellFsClosedir(fd));
	}

	return 0;
}