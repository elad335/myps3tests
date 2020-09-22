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
#include <sysutil/sysutil_gamecontent.h> 

#include <np.h>
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
SYS_PROCESS_PARAM(1000, 0x100000)

u32 callback_count = 0;
std::string dir;

int main(int argc, char* argv[]) {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_GAME );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP_TROPHY );

	CellGameSetInitParams init = {};

	CellFsStat sb;
	cellFunc(FsStat, "/dev_bdvd", &sb);

	for (u32 i = CELL_GAME_GAMETYPE_DISC; i <= CELL_GAME_GAMETYPE_GAMEDATA; i++)
	{
		reset_obj(init);
		std::memcpy(init.title, "hello world", sizeof("hello world"));
		std::memcpy(init.titleId, "FLESS11VM", sizeof("FLESS11VM"));
		std::memcpy(init.version, "03.55", sizeof("03.55"));

		init.titleId[0] += i;

		printf("GAMEDATA type = %d, dir = %s\n", i, init.titleId);

		cellGameDeleteGameData(init.titleId);

		cellFunc(GameDataCheck, i, init.titleId, NULL);

		char path_[129], usrd[129];
		reset_obj(path_, 'x');
		path_[128] = 0;
		reset_obj(usrd, 'x');
		usrd[128] = 0;

		if (g_ec == CELL_GAME_RET_NONE)
		{
			cellFunc(GameCreateGameData, &init, path_, usrd);
		}

		reset_obj(path_, 'x');
		path_[128] = 0;
		reset_obj(usrd, 'x');
		usrd[128] = 0;

		cellFunc(GameContentPermit, path_, usrd);
		printf("%s\n" ,path_);
		printf("%s\n" ,usrd);
	}

	printf("Second:\n");

	for (u32 i = CELL_GAME_GAMETYPE_HDD; i <= CELL_GAME_GAMETYPE_GAMEDATA; i++)
	{
		reset_obj(init);
		std::memcpy(init.title, "hello world", sizeof("hello world"));
		std::memcpy(init.titleId, "FLESS11VM", sizeof("FLESS11VM"));
		std::memcpy(init.version, "03.55", sizeof("03.55"));

		init.titleId[0] += CELL_GAME_GAMETYPE_GAMEDATA;

		printf("GAMEDATA type = %d, dir = %s\n", i, init.titleId);

		cellFunc(GameDataCheck, i, init.titleId, NULL);

		char path_[129], usrd[129];
		reset_obj(path_, 'x');
		path_[128] = 0;
		reset_obj(usrd, 'x');
		usrd[128] = 0;

		cellFunc(GameContentPermit, path_, usrd);
		printf("%s\n" ,path_);
		printf("%s\n" ,usrd);
	}
	return 0;
}