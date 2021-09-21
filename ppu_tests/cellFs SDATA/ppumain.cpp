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

int main() {

	CellSysCacheParam param = {};

	CacheMount(param);
	cellSysCacheClear();

	std::string strtemp(param.getCachePath);
	strtemp += "/temp.txt";
	const char* sdata_file_path = "/app_home/data.sdata";

	int src_fd = open_file(sdata_file_path, CELL_FS_O_CREAT | CELL_FS_O_RDONLY);
	int temp_fd = open_file(strtemp.c_str(), CELL_FS_O_CREAT | CELL_FS_O_RDWR);

	CellFsStat stat;
	ENSURE_OK(cellFsFstat(src_fd, &stat));

	std::string file_buf;
	file_buf.resize(stat.st_size);

	ENSURE_VAL(read_str_file(src_fd, file_buf), file_buf.size());
	
	ENSURE_OK(cellFsClose(src_fd));

	u64 written = 0;
	ENSURE_VAL(write_str_file(temp_fd, file_buf), stat.st_size);
	ENSURE_OK(cellFsClose(temp_fd));

	int norm_fd = open_file(strtemp.c_str(), CELL_FS_O_RDONLY);
	int norm_rw_fd = open_file(strtemp.c_str(), CELL_FS_O_RDWR);
	int sdata_fd = open_sdata(strtemp.c_str(), CELL_FS_O_RDWR);

	CellFsStat sb[4] = {};

	sb[0] = stat_file(norm_fd, true);
	sb[1] = stat_file(sdata_fd, true);

	printf("diff:\n");
	xor_obj(sb[3], sb[0], sb[1]);
	print_obj(sb[3]);

	int sdata_fd_x2 = -1;
	cellFunc(FsSdataOpenByFd, norm_rw_fd, CELL_FS_O_RDWR, &sdata_fd_x2, 0, NULL, 0);
	cellFunc(FsSdataOpenByFd, norm_rw_fd, CELL_FS_O_RDONLY, &sdata_fd_x2, 0, NULL, 0);

	sb[2] = stat_file(sdata_fd_x2, true);

	printf("diff2:\n");
	xor_obj(sb[3], sb[1], sb[2]);
	print_obj(sb[3]);

	printf("Bytes written: 0x%x\n", write_file(-1, &file_buf[0], 0));

	printf("Bytes written: 0x%x\n", write_file(norm_fd, &file_buf[0], 0));
	printf("Bytes written: 0x%x\n", write_file(norm_fd, &file_buf[0], 1));
	printf("Bytes written: 0x%x\n", write_file(sdata_fd, &file_buf[0], 0));
	printf("Bytes written: 0x%x\n", write_file(sdata_fd, &file_buf[0], 1));
	printf("Bytes written: 0x%x\n", write_file(sdata_fd_x2, &file_buf[0], 0));
	printf("Bytes written: 0x%x\n", write_file(sdata_fd_x2, &file_buf[0], 1));

	printf("Bytes written: 0x%x\n", write_off_file(norm_fd, &file_buf[0], 0, 0));
	printf("Bytes written: 0x%x\n", write_off_file(norm_fd, &file_buf[0], 1, 0));
	printf("Bytes written: 0x%x\n", write_off_file(sdata_fd, &file_buf[0], 0, 0));
	printf("Bytes written: 0x%x\n", write_off_file(sdata_fd, &file_buf[0], 1, 0));
	printf("Bytes written: 0x%x\n", write_off_file(sdata_fd_x2, &file_buf[0], 0, 0));
	printf("Bytes written: 0x%x\n", write_off_file(sdata_fd_x2, &file_buf[0], 1, 0));

	return 0;
}