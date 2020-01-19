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

#define cache_id  "16-45-01-83-01000000000000000000"

int main(int argc, const char * const argv[]) {

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

	sys_memory_t mem_id;
	sys_addr_t addr1;
	ENSURE_OK(sys_mmapper_allocate_address(0x20000000, SYS_MEMORY_PAGE_SIZE_64K, 0x10000000, &addr1));
	ENSURE_OK(sys_mmapper_allocate_memory(0x10000, SYS_MEMORY_GRANULARITY_64K, &mem_id));
	ENSURE_OK(sys_mmapper_map_memory(addr1, mem_id, SYS_MEMORY_PROT_READ_WRITE));

	printf("Allocated 0x10000 bytes at address 0x%x\n", addr1);

	// First is allocated, second is not
	void* ptrs[2] = { ptr_cast(addr1), ptr_cast(addr1 + 0x10000) };

	for (int i = 0; i < sizeof(ptrs) / sizeof(ptrs[0]); i++)
	{
		cellSysCacheClear();

		int fd = -1;
		ENSURE_OK(cellFsOpen(strtemp.c_str(), CELL_FS_O_CREAT | CELL_FS_O_RDWR, &fd, NULL, 0));

		if (false)
		{
			const int page_type = CELL_FS_IO_BUFFER_PAGE_SIZE_64KB;
			const size_t container_size = 1024 * 1024;
			sys_memory_container_t container = SYS_MEMORY_CONTAINER_ID_INVALID;
			ENSURE_OK(sys_memory_container_create(&container, container_size));
			const size_t buffer_size = 1024 * 1024;
			cellFunc(FsSetIoBuffer,fd, buffer_size, page_type, container);
		}

		void* const buf = ptrs[i];
		printf("Using address 0x%x\n", int_cast(buf));
		u64 nread = 1ull << 63; // Some random high value
		cellFunc(FsWrite, fd, buf, 0x10000, &nread);
		printf("cellFsWrite(nbytes=0x20000): written size = 0x%llx\n", nread);
		nread = 1ull << 63;
		cellFunc(FsWrite, fd, buf, ~0ull, &nread);
		printf("cellFsWrite(nbytes=UINT64_MAX): written size = 0x%llx\n", nread);
		cellFunc(FsWriteWithOffset, fd, 0, buf, ~0ull, &nread);
		printf("cellFsWriteWithOffset(offset=0, nbytes=UINT64_MAX): written size = 0x%llx\n", nread);
		ENSURE_OK(cellFsFsync(fd));
		CellFsStat stat;
		ENSURE_OK(cellFsFstat(fd, &stat));
		printf("cellFsFStat(): file size = 0x%llx\n", stat.st_size);
		ENSURE_OK(cellFsLseek(fd, 0, 0, &nread));
		ENSURE_OK(nread != 0);
		nread = 1ull << 63;
		cellFunc(FsRead, fd, buf, stat.st_size, &nread);
		printf("cellFsRead(nbytes=0x%llx): read size = 0x%llx\n", stat.st_size, nread);
		ENSURE_OK(cellFsLseek(fd, 0, 0, &nread));
		ENSURE_OK(nread != 0);
		nread = 1ull << 63;
		cellFunc(FsRead, fd, buf, ~0ull, &nread);
		printf("cellFsRead(nbytes=UINT64_MAX): read size = 0x%llx\n", nread);
		cellFunc(FsReadWithOffset, fd, 0, buf, ~0ull, &nread);
		printf("cellFsReadWithOffset(offs=0, nbytes=UINT64_MAX): read size = 0x%llx\n", nread);

		ENSURE_OK(cellFsClose(fd));
	}
	return 0;
}