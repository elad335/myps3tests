#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_savedata.h> 
#include <sysutil/sysutil_syscache.h>
#include <sysutil/sysutil_userinfo.h> 
#include <cell/sysmodule.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <cell/gcm.h>
#include <memory>
#include <sys/vm.h> 

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 1M
SYS_PROCESS_PARAM(1000, 0x100000)

CellSaveDataAutoIndicator ind;
static u32 iteration = 0;

void funcStat(CellSaveDataCBResult *cbResult, CellSaveDataStatGet *get, CellSaveDataStatSet *set)
{
	printf("FuncStat() called! bind=0x%x, sizeKB=0x%x, newData=%d, hddFree=0x%x\n", get->bind, get->sizeKB, get->isNewData
	, get->hddFreeSizeKB);

	cbResult->result = 0;

	set->setParam = iteration == 1 || iteration == 3 ? &get->getParam : NULL;
	set->reCreateMode =  iteration >= 2 ? CELL_SAVEDATA_RECREATE_YES : CELL_SAVEDATA_RECREATE_NO_NOBROKEN;

	switch (iteration)
	{
	case 0: printf("Using CELL_SAVEDATA_RECREATE_NO_NOBROKEN, setParam is NULL, directory doesnt exists\n"); break;
	case 1: printf("Using CELL_SAVEDATA_RECREATE_NO_NOBROKEN, setParam is set, directory doesnt exists\n"); break;
	case 2: printf("Using CELL_SAVEDATA_RECREATE_YES, setParam is NULL\n"); break;
	case 3: printf("Using CELL_SAVEDATA_RECREATE_YES, setParam is set, checking stats"); break;
	}

	//set->setParam->reserved2[0] = (char)0xff; // Error
	//set->setParam->attribute = -1; // Invalid, error
	//set->setParam->reserved[0] = (char)0xff; error
}

void funcFile(CellSaveDataCBResult *cbResult, CellSaveDataFileGet *get, CellSaveDataFileSet *set)
{
	printf("FuncFile() called!\n");
	cbResult->result = 1;
} 

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_SAVEDATA);

	CellSaveDataSetBuf setBuf = {
	1, // dirListMax;
	1, //fileListMax;
	{0}, // Reserved
	sizeof(CellSaveDataDirList) * 10, // bufSize
	NULL, // buf
	};

	setBuf.buf = malloc(setBuf.bufSize);

	cellFunc(SaveDataUserAutoLoad, 0, 0, "BLUS12345", 0, &setBuf, funcStat, funcFile, 0xffffffff, NULL);
	iteration++;
	sys_timer_usleep(500);
	cellFunc(SaveDataUserAutoLoad, 0, 0, "BLUS12345", 0, &setBuf, funcStat, funcFile, 0xffffffff, NULL);
	iteration++;
	sys_timer_usleep(500);
	cellFunc(SaveDataUserAutoLoad, 0, 0, "BLUS12345", 0, &setBuf, funcStat, funcFile, 0xffffffff, NULL);
	iteration++;
	sys_timer_usleep(500);
	cellFunc(SaveDataUserAutoLoad, 0, 0, "BLUS12345", 0, &setBuf, funcStat, funcFile, 0xffffffff, NULL);
 	return 0;
}