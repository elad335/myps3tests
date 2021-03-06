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

#define CELL_GCM_CURRENT CELL_GCM_THIS->current
# define CELL_GCM_RESERVE(count) // CELL_GCM_THIS += (count)
void cellGcm_SetDrawIndexArray(CellGcmContextData * CELL_GCM_THIS, uint8_t mode, 
	uint32_t count, uint8_t type, uint8_t location, uint32_t indicies)
{
	uint32_t startOffset;
	uint32_t startIndex;
	uint32_t misalignedIndexCount;

	CELL_GCM_ASSERT((location <= CELL_GCM_LOCATION_MAIN));
	CELL_GCM_ASSERT((indicies & 0xe0000000) == 0);
	startOffset = (indicies&0x1fffffff);
	/* alignment restriction, SET_INDEX_ARRAY_ADDRESS needs to be 2Byte alignment */
	CELL_GCM_ASSERT((startOffset & 1) == 0);

	// need to compute the number of indexes from starting
	// address to next 128-byte alignment

	// type == 32
	if(type == CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32)
		misalignedIndexCount = (((startOffset + 127) & ~127) - startOffset) >> 2;
	// type == 16
	else
		misalignedIndexCount = (((startOffset + 127) & ~127) - startOffset) >> 1;

	CELL_GCM_RESERVE(7);

	CELL_GCM_METHOD_INVALIDATE_VERTEX_FILE(CELL_GCM_CURRENT);

	// begin
	CELL_GCM_METHOD_SET_INDEX_ARRAY_OFFSET_FORMAT(CELL_GCM_CURRENT, 
		CELL_GCM_COMMAND_CAST(location), startOffset, CELL_GCM_COMMAND_CAST(type));
	CELL_GCM_METHOD_SET_BEGIN_END(CELL_GCM_CURRENT, 
		CELL_GCM_COMMAND_CAST(mode));

	CELL_GCM_METHOD_SET_BEGIN_END(CELL_GCM_CURRENT, 
		CELL_GCM_COMMAND_CAST(mode | 0xff));

	startIndex = 0;
	// starting address of first index is not 128 byte aligned
	// send the mis-aligned indices thus aligning the rest to 128 byte boundary
	if (misalignedIndexCount && (misalignedIndexCount < count))
	{
		uint32_t tmp = misalignedIndexCount-1;
		CELL_GCM_RESERVE(2);
		CELL_GCM_METHOD_DRAW_INDEX_ARRAY(CELL_GCM_CURRENT, startIndex,tmp);
		count -= misalignedIndexCount;
		startIndex += misalignedIndexCount;
	}

	// avoid writing more then 2047(0x7ff) words per inc method (hw limit)
	CELL_GCM_ASSERT(count && (count <= 0xfffff)); // hw limit
	while(count > 0x7FF00)
	{
		CELL_GCM_RESERVE(1+CELL_GCM_MAX_METHOD_COUNT);

		count -= 0x7ff00;
		CELL_GCM_CURRENT[0] = CELL_GCM_METHOD_NI(CELL_GCM_NV4097_DRAW_INDEX_ARRAY, CELL_GCM_MAX_METHOD_COUNT);
		CELL_GCM_CURRENT += 1;
		for (uint32_t lcount = CELL_GCM_MAX_METHOD_COUNT; lcount; --lcount)
		{
			CELL_GCM_CURRENT[0] = CELL_GCM_ENDIAN_SWAP(0xFF000000 | startIndex);
			CELL_GCM_CURRENT += 1;
			startIndex += 0x100;
		}
	}

	// round up count to 256(0x100) counts
	uint32_t mcount = (count + 0xff)>>8;

	CELL_GCM_RESERVE(1+mcount);

	// [startIndex, startIndex+0xff] range in DRAW_INDEX_ARRAY
	CELL_GCM_CURRENT[0] = CELL_GCM_METHOD_NI(CELL_GCM_NV4097_DRAW_INDEX_ARRAY, mcount);
	CELL_GCM_CURRENT += 1;
	while(count > 0x100)
	{
		count -= 0x100;
		CELL_GCM_CURRENT[0] = CELL_GCM_ENDIAN_SWAP(0xFF000000 | startIndex);
		CELL_GCM_CURRENT += 1;
		startIndex += 0x100;
	}

	// remainder indices
	if(count)
	{
		--count;
		CELL_GCM_CURRENT[0] = CELL_GCM_ENDIAN_SWAP((count << 24) | startIndex);
		CELL_GCM_CURRENT += 1;
	}

	CELL_GCM_RESERVE(1);
	//CELL_GCM_METHOD_SET_BEGIN_END(CELL_GCM_CURRENT, 0);

	CELL_GCM_DEBUG_FINISH(CELL_GCM_THIS);
}

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
	gcmLabel start = c.newLabel();
	{
		uint16_t x,y,w,h;
		float min, max;
		float scale[4],offset[4];

		x = 0;
		y = 0;
		w = 1280;
		h = 720;
		min = 0.0f;
		max = 1.0f;
		scale[0] = w * 0.5f;
		scale[1] = h * -0.5f;
		scale[2] = (max - min) * 0.5f;
		scale[3] = 0.0f;
		offset[0] = x + scale[0];
		offset[1] = y + h * 0.5f;
		offset[2] = (max + min) * 0.5f;
		offset[3] = 0.0f;

		cellGcmSetViewport(&Gcm, x, y, w, h, min, max, scale, offset);
		cellGcmSetScissor(&Gcm, x, y, w, h);
	}

	static const u32 colorOffset = 0x400000;
	static const u32 depthOffset = 0xF00000;
	static const u32 texOffset = 0x900000;

	CellGcmSurface surface;
	surface.type = CELL_GCM_SURFACE_PITCH;
	surface.antialias = CELL_GCM_SURFACE_CENTER_1;
	surface.colorFormat = CELL_GCM_SURFACE_A8R8G8B8;
	surface.colorTarget = CELL_GCM_SURFACE_TARGET_0;
	surface.colorLocation[0] = CELL_GCM_LOCATION_LOCAL;
	surface.colorOffset[0] = colorOffset;
	surface.colorPitch[0] = 1280*4;
	surface.colorLocation[1] = CELL_GCM_LOCATION_LOCAL;
	surface.colorOffset[1] = 0;
	surface.colorPitch[1] = 64;
	surface.colorLocation[2] = CELL_GCM_LOCATION_LOCAL;
	surface.colorOffset[2] = 0;
	surface.colorPitch[2] = 64;
	surface.colorLocation[3] = CELL_GCM_LOCATION_LOCAL;
	surface.colorOffset[3] = 0;
	surface.colorPitch[3] = 64;
	surface.depthFormat = CELL_GCM_SURFACE_Z16;
	surface.depthLocation = CELL_GCM_LOCATION_LOCAL;
	surface.depthOffset = depthOffset;
	surface.depthPitch = 1280*2;
	surface.width = 1280;
	surface.height = 720;
	surface.x = 0;
	surface.y = 0;
	cellGcmSetSurface(&Gcm, &surface);

	//Get Cg Parameters
	{
		CGparameter objCoord =
			cellGcmCgGetNamedParameter( vpFile,
										"a2v.objCoord" );
		ENSURE_NVAL(objCoord, 0);

		CGparameter texCoord =
			cellGcmCgGetNamedParameter( vpFile,
										"a2v.texCoord" );
		ENSURE_NVAL(texCoord, 0);

		CGparameter texture =
			cellGcmCgGetNamedParameter( fpFile,
										"texture" );
		ENSURE_NVAL(texture, 0);

		g_obj_coord_idx =
			( cellGcmCgGetParameterResource( vpFile,
											 objCoord) - CG_ATTR0 );
		g_tex_coord_idx =
			( cellGcmCgGetParameterResource( vpFile,
											 texCoord) - CG_ATTR0 );
		g_tex_sampler =
			( cellGcmCgGetParameterResource( fpFile,
											 texture ) - CG_TEXUNIT0 );
	}

	cellGcmSetTextureControl(&Gcm, g_tex_sampler, CELL_GCM_TRUE, 12, 0, CELL_GCM_TEXTURE_MAX_ANISO_1);

	cellGcmSetTextureAddress( &Gcm, g_tex_sampler, 
							  CELL_GCM_TEXTURE_WRAP,
							  CELL_GCM_TEXTURE_WRAP,
							  CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
							  CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL, 
							  CELL_GCM_TEXTURE_ZFUNC_LESS, 0);
	cellGcmSetTextureFilter( &Gcm, g_tex_sampler, 0,
							 CELL_GCM_TEXTURE_NEAREST,
							 CELL_GCM_TEXTURE_NEAREST, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);

	CellGcmTexture tex;
	tex.location = CELL_GCM_LOCATION_MAIN;
	tex.format = CELL_GCM_TEXTURE_A8R8G8B8 | CELL_GCM_TEXTURE_NR | CELL_GCM_TEXTURE_LN;
	tex.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	tex.pitch = 1280*4;
	tex.height = 720;
	tex.width = 720;
	tex.offset = texOffset;
	tex.depth = 1;
	tex.cubemap = CELL_GCM_FALSE;
	tex.mipmap = 1;
	tex.remap = CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
	CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
	CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
	CELL_GCM_TEXTURE_REMAP_REMAP << 8 |
	CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
	CELL_GCM_TEXTURE_REMAP_FROM_G << 4 |
	CELL_GCM_TEXTURE_REMAP_FROM_R << 2 |
	CELL_GCM_TEXTURE_REMAP_FROM_A;

	{
		u32 i = 0;
		for (u32* fill = OffsetToAddr(texOffset); i < 1280 * 720; i++, fill++)
		{
			// Grayish color
			*fill = 0xFF8F8F8Fu;
		}
	}

	{
		u32 i = 0;
		for (u32* fill = (u32*)gLocations::video; i < 1280 * 720; i++, fill++)
		{
			*fill = -1u;
		}
	}

	cellGcmSetTexture(&Gcm, g_tex_sampler, &tex);

	// State setting
	cellGcmSetShadeMode(&Gcm, CELL_GCM_SMOOTH);

	cellGcmSetVertexProgram(&Gcm, vpFile, vpUcode);
	cellGcmSetFragmentProgram(&Gcm, fpFile, 0x0900000);

	cellGcmSetVertexDataArray(&Gcm, g_obj_coord_idx, 0, sizeof(VertexData3D), 3, 
						  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 
						  0x0A00000u ); 
	cellGcmSetVertexDataArray(&Gcm, g_tex_coord_idx, 0, sizeof(VertexData3D), 2, 
						  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 
						  0x0A00000u + (sizeof(float) * 4) ); 

	cellGcmSetBlendEnable(&Gcm, 1);
	cellGcmSetBlendEquation(&Gcm, CELL_GCM_MIN, CELL_GCM_MAX);
	cellGcmSetBlendEquation(&Gcm, 0, 0);
	cellGcmSetInvalidateTextureCache(&Gcm, CELL_GCM_INVALIDATE_TEXTURE);
	cellGcmSetInvalidateVertexCache(&Gcm);
	gfxFence(&Gcm);
	c.debugBreak();


//#define USE_16_BIT_INDEX
#ifndef USE_16_BIT_INDEX
	u32* indexes = ptr_caste(0xc4900000, u32);
#else
	u16* indexes = ptr_caste(0xc4900000, u16);
#endif
	indexes[0] = 0;
	indexes[1] = 1;
	indexes[2] = 2;
	indexes[3] = 3;
	indexes[4] = 4; // Extra index provided in case the hw does alignment up instead of down
	{
#ifndef USE_16_BIT_INDEX
	const u32 type = CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32;
	const u32 offs = 0x04900003; // Unaligned
#else
	const u32 type = CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16;
	const u32 offs = 0x04900001; // Unaligned
#endif
	cellGcmSetDrawIndexArray(&Gcm, CELL_GCM_PRIMITIVE_TRIANGLE_STRIP, 4,type,CELL_GCM_LOCATION_LOCAL, offs);
	
	}
	gcmLabel displaySync = c.newLabel();
	cellGcmSetFlip(&Gcm, id);
	cellGcmSetWaitFlip(&Gcm);
	cellGcmSetReferenceCommand(&Gcm, 2);
	c.jmp(displaySync);

	c.flush();
	sys_timer_usleep(100);

	while (true || load_vol(ctrl->ref) != 2) 
	{
		sys_timer_usleep(1000);
	}

	printf("sample finished.\n");

	return 0;
}