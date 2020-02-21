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
void funcStat(
 CellGameDataCBResult *cbResult,
 CellGameDataStatGet *get,
 CellGameDataStatSet *set)
{
	printf("get:\n");
	printBytes(get, sizeof (*get));
	printf("filepget=%p, filpset=%p\n", &get->getParam, set->setParam);

	if (callback_count)
	{
		set->setParam = &get->getParam;
	}

	cbResult->result = 0;
	return;
}
int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_GAME );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP_TROPHY );

	sceNpTrophyInit(NULL, 0, SYS_MEMORY_CONTAINER_ID_INVALID, 0);

	u32 container = 0;
	ENSURE_OK(sys_memory_container_create(&container, 6<<20));

	CellDiscGameSystemFileParam getParam[1];
	memset(getParam, 0, sizeof getParam);
	cellFunc(DiscGameGetBootDiscInfo, getParam );
	printBytes(getParam, sizeof getParam);

	memset(getParam, 0xff, sizeof getParam);
	cellFunc(DiscGameGetBootDiscInfo, getParam );
	printBytes(getParam, sizeof getParam);

	for (;callback_count < 3; callback_count++) cellFunc(GameDataCheckCreate, 0, "NPEB10097", 0, funcStat, container);


	return 0;
}