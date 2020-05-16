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

/*
#define CELL_GCM_FUNCTION_MACROS
#include "cell/gcm/gcm_function_macros.h"
#include "cell/gcm/gcm_methods.h"
#define CELL_GCM_THIS thisContext
#define CELL_GCM_CURRENT CELL_GCM_THIS->current
#define CELL_GCM_END CELL_GCM_THIS->end
#define CELL_GCM_CALLBACK CELL_GCM_THIS->callback

CELL_GCM_DECL void CELL_GCM_FUNC(SetWaitLabel1)(CELL_GCM_ARGS(uint8_t index, uint32_t value))
{
	CELL_GCM_ASM_IN();
	CELL_GCM_ASM_RESERVE_IMM(4, 2);

	uint32_t offset = 0x10 * index //;
	 + 1;
	CELL_GCM_METHOD_CHANNEL_SEMAPHORE_OFFSET(CELL_GCM_CURRENT, offset);
	CELL_GCM_METHOD_CHANNEL_SEMAPHORE_ACQUIRE(CELL_GCM_CURRENT, value);

	CELL_GCM_DEBUG_FINISH(CELL_GCM_THIS);
	CELL_GCM_ASM_OUT();
}

CELL_GCM_DECL void CELL_GCM_FUNC(SetWriteCommandLabel1)(CELL_GCM_ARGS(
	uint8_t index, uint32_t value))
{
	CELL_GCM_ASM_IN();
	CELL_GCM_ASM_RESERVE_IMM(4, 2);

	uint32_t offset = 0x10 * index //;
	 + 1;
	CELL_GCM_METHOD_CHANNEL_SEMAPHORE_OFFSET(CELL_GCM_CURRENT, offset);
	CELL_GCM_METHOD_CHANNEL_SEMAPHORE_RELEASE(CELL_GCM_CURRENT, value);

	CELL_GCM_DEBUG_FINISH(CELL_GCM_THIS);
	CELL_GCM_ASM_OUT();
}

CELL_GCM_DECL void CELL_GCM_FUNC(SetWriteBackEndLabel1)(CELL_GCM_ARGS(uint8_t index, uint32_t value))
{
	CELL_GCM_ASM_IN();
	CELL_GCM_ASM_RESERVE_IMM(4, 2);

	// swap byte 0 and 2
	uint32_t war_value = value;
	war_value = ( war_value & 0xff00ff00)
		| ((war_value >> 16) & 0xff)
		| (((war_value >> 0 ) & 0xff) << 16);

	uint32_t offset = 0x10 * index //;
	 + 1;
	CELL_GCM_METHOD_SET_SEMAPHORE_OFFSET(CELL_GCM_CURRENT, offset);
	CELL_GCM_METHOD_BACK_END_WRITE_SEMAPHORE_RELEASE(CELL_GCM_CURRENT, war_value);

	CELL_GCM_DEBUG_FINISH(CELL_GCM_THIS);
	CELL_GCM_ASM_OUT();
}

CELL_GCM_DECL void CELL_GCM_FUNC(SetWriteTextureLabel1)(CELL_GCM_ARGS(uint8_t index, uint32_t value))
{
	CELL_GCM_ASM_IN();
	CELL_GCM_ASM_RESERVE_IMM(4, 2);
	uint32_t offset = 0x10 * index // ;
	 + 1;
#ifdef __SPU__
	uint32_t *ptr = CELL_GCM_CURRENT;
	uint32_t *nptr = CELL_GCM_CURRENT + 4;
	uint32_t ptroffset = (uint32_t)ptr & 0xf;
	vec_uint4 *vptr0 = (vec_uint4*)((uintptr_t)ptr);
	vec_uint4 *vptr1 = (vec_uint4*)((uintptr_t)nptr);
	vec_uint4 dstVec0 = *vptr0;
	vec_uint4 dstVec1 = *vptr1;
	CELL_GCM_CURRENT = nptr; 
	vec_uint4 src0 = (vec_uint4){CELL_GCM_METHOD(CELL_GCM_NV4097_SET_SEMAPHORE_OFFSET, 1),
								 (offset),
								 CELL_GCM_METHOD(CELL_GCM_NV4097_TEXTURE_READ_SEMAPHORE_RELEASE, 1),
								 (value)};
	vec_uint4 mask = (vec_uint4)spu_splats(0xffffffff);
	vec_uint4 mask0 = (vec_uint4)spu_rlmaskqwbyte(mask, -ptroffset);
	vec_uint4 val0 = spu_rlmaskqwbyte(src0, -ptroffset);
	vec_uint4 val1 = spu_slqwbyte(src0, 16 - ptroffset);
	*vptr0 = spu_sel(dstVec0, val0, mask0);
	*vptr1 = spu_sel(val1, dstVec1, mask0);
#else
	CELL_GCM_METHOD_SET_SEMAPHORE_OFFSET(CELL_GCM_CURRENT, offset);
	CELL_GCM_METHOD_TEXTURE_READ_SEMAPHORE_RELEASE(CELL_GCM_CURRENT, value);
#endif
	CELL_GCM_DEBUG_FINISH(CELL_GCM_THIS);
	CELL_GCM_ASM_OUT();
}

#undef CELL_GCM_FUNCTION_MACROS
#include "cell/gcm/gcm_function_macros.h"
*/
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
 	cellGcmSetDisplayBuffer(id, 2<<20, 1280*4, 1280, 720);
	cellGcmGetOffsetTable(&offsetTable);

	// Place a jump into io address 1mb
	*OffsetToAddr(ctrl->get) = (1<<20) | RSX_METHOD_NEW_JUMP_CMD;
	sys_timer_usleep(40);

	// Get shaders' ucode address and size
	{
		cellGcmCgGetUCode(vpFile, &vpUcode, &vpUcodeSize);
		cellGcmCgGetUCode(fpFile, &fpUcode, &fpUcodeSize);

		// Load FP program ucode into local memory
		memcpy(gLocations::fragment, fpUcode, fpUcodeSize);
	}

	struct VertexData3D { float i[6];} __attribute__ ((aligned(16)));

#define MINX -1.0f
#define MINY -1.0f
#define MAXX 1.0f
#define MAXY 1.0f
	static VertexData3D vertices[4] __attribute__ ((aligned(16))) = {
		  // vertex		   tex coord
		{ MINX,MINY,0.f,0,	0.f, 0.f },
		{ MAXX,MINY,0.f,0,	1.f, 0.f },
		{ MINX,MAXY,0.f,0,	0.f, 1.f },
		{ MAXX,MAXY,0.f,0,	1.f, 1.f }
	};
#undef MINX
#undef MINY
#undef MAXX
#undef MAXY

	// Initial vertex array
	memcpy(gLocations::varray, &vertices, sizeof(VertexData3D) * 4);

	cellGcmSetupContextData(&Gcm, ptr_caste(addr + (1<<20), u32), 0x60000, GcmCallback);

	printf("First buf:\n");

	print_bytes(cellGcmGetLabelAddress(64), 16*6);
	memset(cellGcmGetLabelAddress(64), 0, 16*6);
	fsync();

	cellGcmSetWriteBackEndLabel(&Gcm, 64, 0x12345678);
	cellGcmSetWriteTextureLabel(&Gcm, 65, 0x12345678 + 1);
	cellGcmSetWriteCommandLabel(&Gcm, 66, 0x12345678 + 2);

	//cellGcmSetWriteBackEndLabel1(&Gcm, 67, 0x12345678);
	//cellGcmSetWriteTextureLabel1(&Gcm, 68, 0x12345678 + 1);
	//cellGcmSetWriteCommandLabel1(&Gcm, 69, 0x12345678 + 2);

	cellGcmSetWaitForIdle(&Gcm);
	c.flush();
	sys_timer_usleep(100);

	while (load_vol(ctrl->put) != load_vol(ctrl->get)) 
	{
		sys_timer_usleep(1000); 
		
		//cellGcmSetFlipImmediate(id); // TODO: Fix this (jarves :hammer:)
	}

	printf("Second buf:\n");
	print_bytes(cellGcmGetLabelAddress(64), 16*3);

	printf("sample finished.\n");

	return 0;
}