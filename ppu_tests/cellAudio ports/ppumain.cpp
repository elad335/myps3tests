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
#include <cell/audio.h> 
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

struct A16 { u64 a[2]; };

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_AUDIO );

	ENSURE_OK(cellAudioInit());

	u32 id;
	u64 key;
	ENSURE_OK(cellAudioCreateNotifyEventQueue(&id, &key));
	ENSURE_OK(cellAudioSetNotifyEventQueue(key));

	u32 port = 0;
	CellAudioPortConfig cfg = {};
	{
		CellAudioPortParam param[1] = {8, 8, 0, 0.0};
		ref_cast((u8*)param + 0x18, u32) = 0xF8FEFC;
		cellAudioPortOpen(param, &port);
		cellAudioPortStart(port); sys_timer_usleep(1000000);
		cellAudioGetPortConfig(port, &cfg);
		printf("cfg\n");
		print_obj(cfg);
		printf("index\n");
		print_obj(ref_cast(cfg.readIndexAddr, A16));
		port = 0xff;
		reset_obj(cfg, 0xff);
	}

	{
		CellAudioPortParam param[1] = {2, 8, 0x1, 0.0};
		ref_cast((u8*)param + 0x18, u32) = 0x11EE640 ;
		cellAudioPortOpen(param, &port);
		cellAudioPortStart(port); sys_timer_usleep(1000000);
		cellAudioGetPortConfig(port, &cfg);
		printf("cfg\n");
		print_obj(cfg);
		printf("index\n");
		print_obj(ref_cast(cfg.readIndexAddr, A16));
		port = 0xff;
		reset_obj(cfg, 0xff);
	}

	{
		CellAudioPortParam param[1] = {8, 16, 0x1000, 1.0};
		ref_cast((u8*)param + 0x18, u32) = 0;
		cellAudioPortOpen(param, &port);
		cellAudioPortStart(port); sys_timer_usleep(1000000);
		cellAudioGetPortConfig(port, &cfg);
		printf("cfg\n");
		print_obj(cfg);
		printf("index\n");
		print_obj(ref_cast(cfg.readIndexAddr, A16));
		port = 0xff;
		reset_obj(cfg, 0xff);
	}

	{
		CellAudioPortParam param[1] = {0, 16, 0x1000, 1.0};
		ref_cast((u8*)param + 0x18, u32) = 0;
		cellAudioPortOpen(param, &port);
		cellAudioPortStart(port); sys_timer_usleep(1000000);
		cellAudioGetPortConfig(port, &cfg);
		printf("cfg\n");
		print_obj(cfg);
		printf("index\n");
		print_obj(ref_cast(cfg.readIndexAddr, A16));
		port = 0xff;
		reset_obj(cfg, 0xff);
	}
}