#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_syscache.h>
#include <cell/sysmodule.h>
#include <cell/camera.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <cell/gem.h>
#include <cell/spurs.h> 
#include <sys/spu_thread_group.h>
#include <sys/spu_thread.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>
#include <sys/event.h>
#include <sys/timer.h>
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

sys_memory_t mem_id;

sys_addr_t addr1;
sys_addr_t addr2;
sys_event_queue_t queue_id;

static void gemSampleSpursStart(CellSpurs2* spurs)
{
	#define SPURS_NUM_SPU 5
	#define SPURS_PPU_THREAD_PRIORITY 1000
	#define PRINTF_PPU_THREAD_PRIORITY 400
	#define SPURS_THREAD_GROUP_PRIORITY 250
	#define SAMPLE_SPURS_PREFIX "GemSample"

	cellSysmoduleInitialize();
	sys_spu_initialize(6, 0);

	CellSpursAttribute attr;
	cellSpursAttributeInitialize(&attr, SPURS_NUM_SPU, SPURS_THREAD_GROUP_PRIORITY, SPURS_PPU_THREAD_PRIORITY, false);
	cellSpursAttributeEnableSpuPrintfIfAvailable(&attr);
	cellSpursAttributeSetNamePrefix(&attr, SAMPLE_SPURS_PREFIX, strlen(SAMPLE_SPURS_PREFIX));
	cellSpursInitializeWithAttribute2(spurs, &attr);
}

int UpdateStart(void* camera_frame, u64 time = 0)
{
	int err = cellGemUpdateStart(camera_frame, time);
	printf("cellGemUpdateStart(0x%x)\n", err);
	return err;
}

int UpdateFinish()
{
	int err = cellGemUpdateFinish();
	printf("cellGemUpdateFinish(0x%x)\n", err);
	return err;
}

int main()
{
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_GEM );
	cellSysmoduleLoadModule( CELL_SYSMODULE_CAMERA );

	static CellSpurs2 gSpurs;
	gemSampleSpursStart(&gSpurs); // libgem uses spurs, so need to start it

	uint8_t gem_spu_priorities[8] = {1,0,0,0,0,0,0,0}; // execute libgem jobs on 1 spu
	CellGemAttribute gem_attr;
	cellGemAttributeInit(&gem_attr, 1, NULL, &gSpurs, gem_spu_priorities);
	cellGemInit(&gem_attr);

	// Test null ptr
	UpdateStart(NULL, 0);
	sys_timer_sleep(1);
	UpdateFinish();


	UpdateStart(NULL, 0);
	sys_timer_sleep(1);
	UpdateStart(NULL, 0);

	printf("sample finished.\n");

	return 0;
}