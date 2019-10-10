#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_sysparam.h> 
#include <sysutil/sysutil_syscache.h>
#include <cell/sysmodule.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <memory>
#include <string.h>

#include "../rsx_header.h"

inline void __check() { asm volatile ("twi 0x10, 3, 0"); };

#define SYS_APP_HOME "/app_home"

#define VP_PROGRAM SYS_APP_HOME "/mainvp.vpo"
#define FP_PROGRAM SYS_APP_HOME "/mainfp.fpo"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack	: 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

static sys_memory_t mem_id;
static sys_addr_t addr;

//shader static data
static char *vpFile = NULL;
static CellCgbProgram vp;
static CellCgbVertexProgramConfiguration vpConf; 
static const void *vpUcode = NULL;

static char *fpFile = NULL;
static CellCgbProgram fp;
static CellCgbFragmentProgramConfiguration fpConf;

static void *vucode, *fucode;
static u32 fpUcodeSize, vpUcodeSize;

static inline void LoadModules()
{
	int ret = cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );
	ret |= cellSysmoduleLoadModule( CELL_SYSMODULE_FS );
	ret |= cellSysmoduleLoadModule( CELL_SYSMODULE_RESC );
	if (ret != CELL_OK && ret != CELL_SYSMODULE_ERROR_DUPLICATED) asm volatile ("tw 4, 1, 1");
}

u32 readFile(const char *filename, char **buffer)
{
	FILE *file = fopen(filename,"rb");
	if (!file)
	{
		printf("couldn't open file %s\n",filename);
		return 0;
	}
	fseek(file,0,SEEK_END);
	size_t size = ftell(file);
	fseek(file,0,SEEK_SET);

	*buffer = new char[size+1];
	fread(*buffer,size,1,file);
	fclose(file);
	
	return size;
}

static rsxCommandCompiler c;
static CellGcmContextData& Gcm = c.c;

// CellGcmContextCallback
int GcmCallback(struct CellGcmContextData *, uint32_t){}

int main() {

	// They should be at the same address (Traps are not gay)
	if (int_cast(&Gcm) != int_cast(&c.c)) asm volatile ("tw 4, 1, 1");

	LoadModules();
	sys_memory_allocate(0x1000000, 0x400, &addr);

	cellGcmInit(1<<16, 0x100000, ptr_cast(addr));
	cellGcmMapEaIoAddress(ptr_cast(addr + (1<<20)), 1<<20, 7<<20);

	u8 id; cellGcmGetCurrentDisplayBufferId(&id);
 	cellGcmSetDisplayBuffer(id, 2<<20, 1280*4, 1280, 720);
	cellGcmGetOffsetTable(&offsetTable);

	CellGcmDisplayInfo* info = const_cast<CellGcmDisplayInfo*>(cellGcmGetDisplayInfo());
	CellGcmControl* ctrl = cellGcmGetControlRegister();

	// Wait for RSX to complete previous operation
	do sys_timer_usleep(200); while (ctrl->get != ctrl->put);

	// Place a jump into io address 1mb
	*OffsetToAddr(ctrl->get) = (1<<20) | RSX_METHOD_NEW_JUMP_CMD;
	sys_timer_usleep(40);

	cellGcmSetupContextData(&Gcm, ptr_caste(addr + (1<<20), u32), 0x10000, GcmCallback);

	// Test 'dirty' return command opcode
	c.push(RSX_METHOD_RETURN_CMD | NV406E_SET_REFERENCE);
	c.push(0);

	cellGcmSetReferenceCommand(&Gcm, 2);
	fsync();

	ctrl->put = c.newLabel().pos;
	sys_timer_usleep(100);

	while (ctrl->ref != 2) sys_timer_usleep(1000);

	printf("sample finished.\n");

	return 0;
}