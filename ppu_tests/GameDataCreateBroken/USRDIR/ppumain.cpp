#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_syscache.h>
#include <cell/sysmodule.h>
#include <sysutil/sysutil_sysparam.h> 
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <cell/gcm.h>
#include <cell/pad.h>
#include <cell/cell_fs.h>
#include <cell/audio.h> 
#include <memory>
#include <iostream>
#include <string>
#include <list>
#include <map>
#include <netex/libnetctl.h> 
#include <np/drm.h> 
#include <np.h>
#include <sysutil/sysutil_music2.h>
#include <sysutil/sysutil_music.h>
#include <sysutil/sysutil_gamecontent.h> 

#include "../../ppu_header.h"
#include "../../libgame_old.h"

#include <cell/fiber/ppu_fiber.h>

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
#define CELL_SDK_VERSION  0x350001
SYS_PROCESS_PARAM(1000, 0x10000)

std::string dir;

u32 callback_count = 0;
CellGameDataSystemFileParam _leak_param = {};
CellGameDataSystemFileParam* leak_param = &_leak_param;

bool callback_advance = false;

void funcStat(
 CellGameDataCBResult *cbResult,
 CellGameDataStatGet *get,
 CellGameDataStatSet *set)
{
	printf("get:\n");
	//print_obj(*get);
	printf(get->getParam.titleId);
	printf("\nfilepget=%p, filpset=%p\n", &get->getParam, set->setParam);

	char* titleId[] =
	{
		"BLES-00159",
		"BLES001591",
		"BLES0015912",
		"bles00159",
		"bles",
		"",
		"Bles",
		"BLES_11",
		"BLES 11",
		"BLES..",
		"..",
		"./",
		".",
		"Title", // Test invalid title
		"TitleL", // Test invalid language specific title
		"pad",
	};

	std::memcpy(leak_param, &get->getParam, sizeof(CellGameDataSystemFileParam));

	set->setParam = leak_param;
	reset_obj(leak_param->titleId);
	strcpy(leak_param->titleId, titleId[callback_count]);
	//leak_param->titleId[10] = '0';

	if (callback_count >= 1 && strcmp(titleId[callback_count - 1], "Title") == 0)
	{
		printf("Poisoning title\n");
		reset_obj(leak_param->title, '0');
	}
	else if (callback_count >= 1 && strcmp(titleId[callback_count - 1], "TitleL") == 0)
	{
		printf("Poisoning titleLang\n");
		reset_obj(leak_param->titleLang, '0');
	}

	callback_advance = callback_count < (sizeof(titleId) / sizeof(char*));
	cbResult->result = 0;
}

int main(int argc, char* argv[]) {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_GAME );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP_TROPHY );

	sceNpTrophyInit(NULL, 0, SYS_MEMORY_CONTAINER_ID_INVALID, 0);
	u32 container = 0;
	ENSURE_OK(sys_memory_container_create(&container, 6<<20));

	cellGameDeleteGameData("BLES00159");
	callback_count = 0;

	do
	{
		cellFunc(GameDataCheckCreate2, 0, "BLES00159", 0, funcStat, container);
		callback_count++;
	}
	while (callback_advance);

	return 0;
}