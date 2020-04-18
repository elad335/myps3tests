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

 // _binary_{SHADERFILENAME}_f/vpo_start loads the shader
extern char _binary_mainvp_vpo_start[];
extern char _binary_mainfp_fpo_start[];

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack	: 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

static sys_memory_t mem_id;
static sys_addr_t addr;

//shader static data
static CGprogram vpFile = (CGprogram)ptr_cast(&_binary_mainvp_vpo_start[0]);;
static CGprogram fpFile = (CGprogram)ptr_cast(&_binary_mainfp_fpo_start[0]);;

static void *vpUcode, *fpUcode;
static u32 fpUcodeSize, vpUcodeSize;

// Cg indexes
static u32 g_obj_coord_idx, g_tex_coord_idx, g_tex_sampler;

static inline void LoadModules()
{
	cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );
	cellSysmoduleLoadModule( CELL_SYSMODULE_FS );
	cellSysmoduleLoadModule( CELL_SYSMODULE_RESC );
}

static rsxCommandCompiler c;
static CellGcmContextData& Gcm = c.c;

// CellGcmContextCallback
int GcmCallback(struct CellGcmContextData *, uint32_t){}

namespace gLocations
{
	void* video = ptr_cast(0xC0200000); // Video
	void* fragment = ptr_cast(0xC0900000); // FP shader
	void* varray = ptr_cast(0xC0A00000); // Vertex array 1
	void* color1 = ptr_cast(0xC0D00000); // Color buffer 0
};

int main() {

	LoadModules();
	sys_memory_allocate(0x1000000, 0x400, &addr);

	ENSURE_OK(cellGcmInit(1<<16, 0x100000, ptr_cast(addr))); 
	CellGcmControl* ctrl = cellGcmGetControlRegister();
	wait_for_fifo(ctrl);
	GcmMapEaIoAddress(addr + (1<<20), 1<<20, 15<<20);

	{
		CellRescInitConfig resConfig;
		resConfig.size = sizeof(CellRescInitConfig);
		resConfig.resourcePolicy = CELL_RESC_MINIMUM_VRAM | CELL_RESC_MINIMUM_GPU_LOAD;
		resConfig.supportModes = CELL_RESC_720x480 | CELL_RESC_720x576 | CELL_RESC_1280x720 | CELL_RESC_1920x1080;
		resConfig.ratioMode = CELL_RESC_FULLSCREEN;
		resConfig.palTemporalMode = CELL_RESC_PAL_50;
		resConfig.interlaceMode = CELL_RESC_NORMAL_BILINEAR;
		resConfig.flipMode = CELL_RESC_DISPLAY_VSYNC;
		cellRescInit(&resConfig);
		if (cellRescSetDisplayMode(CELL_RESC_1280x720)) return 0;
	}

	CellGcmConfig gcm_cfg;
	reset_obj(gcm_cfg);
	cellGcmGetConfiguration(&gcm_cfg);

	const u32 tiled_pitch = cellGcmGetTiledPitchSize(1280*4);
	u8 id; cellGcmGetCurrentDisplayBufferId(&id); u8 zcull_id = 0;
	cellFunc(GcmBindZcull, zcull_id++ % 8, 0, 0, 0, 0, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, 0, 2048 + 64, 1536 + 64, 0, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, 0, 2048 + 64, 64, 0, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, 0, 64, 1536 + 64, 0, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, gcm_cfg.localSize - MB(6), 2048, 64, MB(2), CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, gcm_cfg.localSize - MB(17), 2048, 1536, MB(2), CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, gcm_cfg.localSize, 2048, 1536, 0, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, 0, 2048, 1536, gcm_cfg.localSize - MB(1), CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, 1<<31, 2048, 1536, 0, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, 1<<30, 2048, 1536, 0, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, 1<<29, 2048, 1536, 0, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, 1<<28, 2048, 1536, 0, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, 0, 2048, 1536, 1<<31, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, 0, 2048, 1536, 1<<30, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, 0, 2048, 1536, 1<<29, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);
	cellFunc(GcmBindZcull, zcull_id++ % 8, 0, 2048, 1536, 1<<28, CELL_GCM_ZCULL_Z16, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_MSB,
CELL_GCM_SCULL_SFUNC_NEVER, 0, 0);

	return 0;
}