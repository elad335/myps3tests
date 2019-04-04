#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_savedata.h> 
#include <sysutil/sysutil_syscache.h>
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

void funcStat(CellSaveDataCBResult *cbResult, CellSaveDataStatGet *get, CellSaveDataStatSet *set)
{
	printf("FuncStat() called! bind=0x%x, sizeKB=0x%x, \n", get->bind, get->sizeKB);
	cbResult->result = 0;
	set->indicator = &ind;
	set->setParam = &get->getParam;
	//set->reCreateMode = CELL_SAVEDATA_RECREATE_NO_NOBROKEN; // Error
	set->reCreateMode = CELL_SAVEDATA_RECREATE_YES;
	//set->reCreateMode = CELL_SAVEDATA_RECREATE_YES | (1<<31); // Error
	//set->setParam->reserved2[0] = (char)0xff; // Error
	//set->setParam->attribute = -1; // Invalid, error
	//set->setParam->reserved[0] = (char)0xff; error
}

void funcFile(CellSaveDataCBResult *cbResult, CellSaveDataFileGet *get, CellSaveDataFileSet *set)
{
	printf("FuncFile() called!\n");
	cbResult->result = 1; // Abort prematuraly
} 

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_SAVEDATA);
	int ret = 0;
	CellSaveDataSetBuf setBuf = {
	1, // dirListMax;
	1, //fileListMax;
	{0}, // Reserved
	sizeof(CellSaveDataDirList) * 10, // bufSize
	NULL, // buf
	};

	setBuf.buf = (void*)(new u8[setBuf.bufSize]);

	ret = cellSaveDataUserAutoLoad(0, 0, "BLES00412", 0, &setBuf, funcStat, funcFile, 0xffffffff, NULL);
	printf("ret = 0x%x", ret);
 	return 0;
}