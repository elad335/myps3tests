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
#include <np/drm.h> 
#include <np.h>
#include <sysutil/sysutil_music2.h>
#include <sysutil/sysutil_music.h>

#include "../ppu_header.h"

#include <cell/fiber/ppu_fiber.h>

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;

sys_addr_t addr1;
sys_addr_t addr2;
bool ok = false;

void loop_util()
{
	while (!ok)
	{
		sys_timer_usleep(100000);
		cellSysutilCheckCallback();
	}
	ok = false;
}

extern "C" unsigned long long prx_function_elad();

void validate(int& result)
{
	result = prx_function_elad();
	printf("%d\n", result);
	result = 0;
}

const char* prx_name = "/app_home/used_prx.sprx";

int allocate_prx_memory_untracked()
{
	sysCell(prx_load_module, prx_name, 0, NULL);
	return 0;
}

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_MUSIC );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_MUSIC2 );
	cellSysmoduleLoadModule( CELL_SYSMODULE_AUDIO );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP_TROPHY );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_LICENSEAREA );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP2 );

	static const u64 key[2] = { 0xdca01b0ffd4a2affull , 0x292accffdd5bf861ull };

	u8* buffer = new u8[128 * 1024 * 1024];
	sceFunc(Np2Init, 128 * 1024 * 1024, buffer);

	//ceFunc(NpDrmIsAvailable, (const SceNpDrmKey*)key, "/app_home/UnityNpToolkit.sprx");

	sys_memory_container_t container = 0;
	ENSURE_OK(sys_memory_container_create(&container, CELL_MUSIC_PLAYBACK_MEMORY_CONTAINER_SIZE));


	//sysCell(prx_load_module, "/app_home/UnityNpToolkit.sprx", 0, NULL);
	//sysCell(prx_load_module, "/app_home/././././UnityNpToolkit.sprx", 0, NULL);
	//sysCell(prx_load_module, "/app_home/./UnityNpToolkit.sprx", 0, NULL);
	//sysCell(prx_load_module, "/app_home/UnityNpToolkit.sprx", 0, NULL);

	u32 prx_ids[2]  = {};

	int result = 0;	

	printf("Phase 1:\n\n");

	for (u32 i = 0; i < 2; i++, allocate_prx_memory_untracked())
	{
		prx_ids[i] = sysCell(prx_load_module, prx_name, 0, NULL);
		sysCell(prx_start_module, prx_ids[i], 0, NULL, &result, 0, NULL);
		validate(result);
		sysCell(prx_stop_module, prx_ids[i], 0, NULL, &result, 0, NULL);
		sysCell(prx_unload_module, prx_ids[i], 0, NULL);
	}

	printf("Phase 2:\n\n");

	for (u32 i = 0; i < 2; i++, allocate_prx_memory_untracked())
	{
		prx_ids[i] = sysCell(prx_load_module, prx_name, 0, NULL);
		sysCell(prx_start_module, prx_ids[i], 0, NULL, &result, 0, NULL);
		validate(result);
		sysCell(prx_stop_module, prx_ids[i], 0, NULL, &result, 0, NULL);
		//sysCell(prx_unload_module, prx_ids[i], 0, NULL);
	}

	for (u32 i = 0; i < 2; i++)
	{
		sysCell(prx_unload_module, prx_ids[i], 0, NULL);
	}

	printf("Phase 3:\n\n");

	for (u32 i = 0; i < 2; i++, allocate_prx_memory_untracked())
	{
		prx_ids[i] = sysCell(prx_load_module, prx_name, 0, NULL);
		sysCell(prx_start_module, prx_ids[i], 0, NULL, &result, 0, NULL);
		validate(result);
	}

	for (u32 i = 0; i < 2; i++, allocate_prx_memory_untracked())
	{
		sysCell(prx_stop_module, prx_ids[i], 0, NULL, &result, 0, NULL);
		validate(result);
	}

	for (u32 i = 0; i < 2; i++)
	{
		sysCell(prx_unload_module, prx_ids[i], 0, NULL);
	}

	if (constant_vol<bool, false>())
	{
		// Import
		cellFiberPpuYield();
		result = prx_function_elad();
	}

	printf("finished");
	return 0;
}