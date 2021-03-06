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
// Stack    : 64KB
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
	void* video = ptr_cast(0xC0000000); // Video
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

	u8 id; cellGcmGetCurrentDisplayBufferId(&id);
 	cellGcmSetDisplayBuffer(id, 0<<20, 1280*4, 1280, 720);
	cellGcmGetOffsetTable(&offsetTable);

	// Place a jump into io address 1mb
	*OffsetToAddr(ctrl->get) = (1<<20) | RSX_METHOD_NEW_JUMP_CMD;
	sys_timer_usleep(40);

	cellGcmSetupContextData(&Gcm, ptr_caste(addr + (1<<20), u32), 0x60000, GcmCallback);

	//Call gcm reset
	c.call(0);

	gfxFence(&Gcm);
	cellGcmSetReferenceCommand(&Gcm, 2);

	// !Inavlid!
	c.call(1 << 28);

	// Unaligned
	const u32 endfifo = (c.pos() - 4) + 1;
	store_vol(ctrl->put, endfifo);

	while (load_vol(ctrl->ref) != 2) 
	{
		sys_timer_usleep(4000); 
	}

	// Possibly wait for a crash (executing invalid call)
	sys_timer_sleep(1);

	const u32 put_old = load_vol(ctrl->put);
	u32 put_ugh = 0;

	for (store_vol(ctrl->put, endfifo);;)
	{
		put_ugh = load_vol(ctrl->put);

		// Check if PUT is modified by rsx at all
		if (put_ugh != endfifo && (put_ugh & 3) == 0)
			break;
	}

	for (;;)
	{
		store_vol(ctrl->put, endfifo);
		put_ugh = load_vol(ctrl->put);

		// Check if PUT is not immediately (at the time of write) modified by rsx
		if (put_ugh == endfifo)
			break;
	}

	printf("sample finished. PUT=0x%x, PUT1=0x%x\n", put_old, put_ugh);
	return 0;
}