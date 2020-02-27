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
static u32 iteration = 0, func_ctr = 0;

void funcStat(CellSaveDataCBResult *cbResult, CellSaveDataStatGet *get, CellSaveDataStatSet *set)
{
	printf("FuncStat() called! bind=0x%x, sizeKB=0x%x, newData=%d, hddFree=0x%x\n", get->bind, get->sizeKB, get->isNewData
	, get->hddFreeSizeKB);

	cbResult->result = 0;

	set->setParam = &get->getParam;
	set->reCreateMode =  CELL_SAVEDATA_RECREATE_NO_NOBROKEN;


	//set->setParam->reserved2[0] = (char)0xff; // Error
	//set->setParam->attribute = -1; // Invalid, error
	//set->setParam->reserved[0] = (char)0xff; error
}

u8 filebuf[1000];

void funcFile(CellSaveDataCBResult *cbResult, CellSaveDataFileGet *get, CellSaveDataFileSet *set)
{
	printf("FuncFile() called!\n");
	printf("set:\n");
	printBytes(set, sizeof(*set));
	printf("get:\n");
	printBytes(get, sizeof(*get));

	memset(get->reserved, 0xff, sizeof(get->reserved));
	set->fileType = CELL_SAVEDATA_FILETYPE_NORMALFILE;

	switch (iteration++)
	{
	case 0:
	{
		set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC;
		set->fileName = "MYTEXT.TXT";
		set->fileSize = set->fileBufSize = 1000;
		set->fileOffset = 1;
		set->fileBuf = filebuf;
		break;
	}
	case 1:
	{
		set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE;
		set->fileName = "MYNON.TXT";
		set->fileSize = set->fileBufSize = 1000;
		set->fileOffset = 1;
		set->fileBuf = filebuf;
		break;
	}
	case 2:
	{
		set->fileOperation = CELL_SAVEDATA_FILEOP_READ;
		set->fileName = "MYTEXT.TXT";
		set->fileSize = set->fileBufSize = 1000;
		set->fileOffset = 10;
		set->fileBuf = filebuf;
		break;
	}
	//case 3:
	//{
	//	set->fileOperation = CELL_SAVEDATA_FILEOP_DELETE;
	//	set->fileType = 0;
	//	set->fileName = "MYTEXT.TXT";
	//	set->fileSize = set->fileBufSize = 0;
	//	set->fileOffset = 0;
	//	set->fileBuf = 0;
	//	break;
	//}
	case 4:
	default: cbResult->result = 1; return;
	}

	cbResult->result = 0;
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

	for (func_ctr = 0; func_ctr < 1; func_ctr++)
	{
		cellFunc(SaveDataUserAutoLoad, 0, 0, "BLUS12345", 0, &setBuf, funcStat, funcFile, 0xffffffff, NULL);
		sys_timer_usleep(500);
	}
 	return 0;
}