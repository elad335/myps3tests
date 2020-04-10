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
#define CELL_SDK_VERSION 0x20000
SYS_PROCESS_PARAM(1000, 0x100000)

CellSaveDataAutoIndicator ind;
u32 iteration = 0;
bool done = false;
u32 in_call = 0;
const u32 real_bufsize = sizeof(CellSaveDataDirList) * 20;

CellSaveDataSetBuf setBuf = {
1, // dirListMax;
1, //fileListMax;
{0}, // Reserved
(real_bufsize / 2) - 3, // bufSize
NULL, // buf
};

void funcStat(CellSaveDataCBResult *cbResult, CellSaveDataStatGet *get, CellSaveDataStatSet *set)
{
	printf("FuncStat() called! bind=0x%x, sysSizeKB=%d, sizeKB=0x%x, newData=%d, hddFree=0x%x, fileNum=0%d\n", get->bind, get->sysSizeKB, get->sizeKB, get->isNewData
	, get->hddFreeSizeKB, get->fileNum);

	printf("sertBuf:\n");
	print_bytes(setBuf.buf, real_bufsize);

	cbResult->result = 0;

	set->setParam = &get->getParam;
	set->reCreateMode =  CELL_SAVEDATA_RECREATE_NO_NOBROKEN;


	//set->setParam->reserved2[0] = (char)0xff; // Error
	//set->setParam->attribute = -1; // Invalid, error
	//set->setParam->reserved[0] = (char)0xff; error
}

u8 filebuf[1000];

void funcFixed(CellSaveDataCBResult *cbResult, CellSaveDataListGet *get, CellSaveDataFixedSet *set)
{
	printf("FuncFixed() called! (res=%d, num=%d)\n", cbResult->result, get->dirListNum);

	printf("sertBuf:\n");
	print_bytes(setBuf.buf, real_bufsize);
	std::memset(setBuf.buf, 0xff, real_bufsize);
	//get->dirListNum = 0; // Hack for testing
	//setBuf.bufSize = 1;
	set->dirName = "NEWDI3R47";
	set->option = CELL_SAVEDATA_OPTION_NOCONFIRM;
}

CellSaveDataListNewData newdata = {};

void funcList(CellSaveDataCBResult *cbResult, CellSaveDataListGet *get, CellSaveDataListSet *set)
{
	printf("FuncList() called!\n");
	printf("set:\n");
	print_obj(*set);
	printf("get:\n");
	print_obj(*get);
	cbResult->result = 0;
	set->fixedListNum = 0;
	//get->fixed
	newdata.dirName = "DI3R123245";
	set->newData = & newdata;
}

void funcFile(CellSaveDataCBResult *cbResult, CellSaveDataFileGet *get, CellSaveDataFileSet *set)
{
	printf("FuncFile() called!\n");
	printf("set:\n");
	print_obj(*set);
	printf("get:\n");
	print_obj(*get);

	cbResult->result = 1; return;
	reset_obj(get->reserved, 0xff);

	set->fileType = CELL_SAVEDATA_FILETYPE_NORMALFILE;

	switch (iteration)
	{
	case 1:
	{
		set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC;
		set->fileName = "MYTEXT.TXT";
		set->fileSize = set->fileBufSize = 1000;
		set->fileOffset = 1;
		set->fileBuf = filebuf;
		break;
	}
	case 2:
	{
		set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE;
		set->fileName = "MYNON.TXT";
		set->fileSize = set->fileBufSize = 1000;
		set->fileOffset = 1;
		set->fileBuf = filebuf;
		break;
	}
	case 3:
	{
		set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE;
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
	case 0: cbResult->result = 1; return;
	default: cbResult->result = 1, done = true; return;
	}

	cbResult->result = in_call++;
} 

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_SAVEDATA);

	setBuf.buf = malloc(real_bufsize);


	CellSaveDataSetList setList = {
	0, // dirListMax;
	0, //fileListMax;
	"1",//"NEWPREFIXNOEXIST1",
	NULL, // buf
	};


	for (iteration = 0; iteration < 1; iteration++, in_call = 0)
	{
		std::memset(setBuf.buf, 0xff, real_bufsize);

		//cellFunc(SaveDataUserListSave, 0, 0, &setList, &setBuf, funcList, funcStat, funcFile, 0xffffffff, NULL);
		//cellFunc(cellSaveDataFixedLoad, 0, 0, "NEWDIR12345", 0, &setBuf, funcStat, funcFile, 0xffffffff, NULL);
		cellFunc(SaveDataFixedLoad2, 1, &setList, &setBuf, funcFixed, funcStat, funcFile, 0xffffffff, NULL);
		sys_timer_usleep(500);
	}
 	return 0;
}