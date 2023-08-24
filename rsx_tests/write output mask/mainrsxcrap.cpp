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
static u32 g_obj_coord_idx, g_tex_coord_idx, g_test_reg_idx, g_tex_sampler;

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

static u32 addr_alloc_base = 0x10000;

u32 alloc_addr(u32 size, u32 align = 256)
{
	const u32 new_addr = (addr_alloc_base + align - 1) & (0 - align);
	addr_alloc_base = new_addr + size;
	return new_addr;
}

namespace mem_locs
{
	const u32 __dummy__ = alloc_addr(0x10000); //
	const u32 fifo = alloc_addr(0x100000, 0x10000); // Fifo
	const u32 video = alloc_addr(1280 * 720 * 4); // Video memory
	const u32 fragment = alloc_addr(1024); // FP shader
	const u32 vertices = alloc_addr(1024); // Vertex array 1
	const u32 color1 = alloc_addr(1280 * 720 * 4); // Color buffer 0
	const u32 color2 = alloc_addr(1280 * 720 * 4); // Color buffer 0
	const u32 texture = alloc_addr(1280 * 720 * 4); // Color buffer 0
	const u32 depth = alloc_addr(1280 * 720 * 8); // Color buffer 0
};

// Description: this testcase tests what happens on a write to ones to reserved of color invalid_mask register
// It does this by iterating amd ORis valid masks with a single reserved bit at a time, for completeness it also tests it with valid bits only once
// Results: 
int main()
{
	printf("fifo 0x%x, video 0x%x, fp 0x%x, color1 0x%x, color2 0x%x, texture 0x%x\n", mem_locs::fifo, mem_locs::video, mem_locs::fragment, mem_locs::color1, mem_locs::color2, mem_locs::texture);
	LoadModules();

	sys_memory_allocate(MB(0x10), 0x400, &addr);

	ENSURE_OK(cellGcmInit(0x10000, MB(1), ptr_cast(addr)));
	CellGcmControl* ctrl = cellGcmGetControlRegister();
	wait_for_fifo(ctrl);
	GcmMapEaIoAddress(addr + mem_locs::fifo, mem_locs::fifo, MB(15));

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

	u8 id = -1;
	cellGcmGetCurrentDisplayBufferId(&id);
 	cellGcmSetDisplayBuffer(id, mem_locs::video, 1280*4, 1280, 720);
	cellGcmGetOffsetTable(&offsetTable);

	// Place a jump into io address 1mb
	*OffsetToAddr(ctrl->get) = mem_locs::fifo | RSX_METHOD_NEW_JUMP_CMD;
	sys_timer_usleep(40);

	// Get shaders' ucode address and size
	{
		cellGcmCgGetUCode(vpFile, &vpUcode, &vpUcodeSize);
		cellGcmCgGetUCode(fpFile, &fpUcode, &fpUcodeSize);

		// Load FP program ucode into local memory
		std::memcpy(OffsetToLocal(mem_locs::fragment), fpUcode, fpUcodeSize);
	}

	struct VertexData3D { float i[5]; } ;

#define MINX -1.0f
#define MINY -1.0f
#define MAXX 1.0f
#define MAXY 1.0f
	static VertexData3D vertices[4] ALIGN(128) =
	{
		  // vertex		   tex coord
		{ MINX,MINY,0.f,	0.f, 0.f },
		{ MAXX,MINY,0.f,	1.f, 0.f },
		{ MINX,MAXY,0.f,	0.f, 1.f },
		{ MAXX,MAXY,0.f,	1.f, 1.f }
	};
#undef MINX
#undef MINY
#undef MAXX
#undef MAXY

	const u32 valid_masks[] =
	{
		CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B | CELL_GCM_COLOR_MASK_A,
		0,
		CELL_GCM_COLOR_MASK_R,
		CELL_GCM_COLOR_MASK_G,
		CELL_GCM_COLOR_MASK_B,
		CELL_GCM_COLOR_MASK_A,
	};

	// Initial vertex array
	std::memcpy(OffsetToLocal(mem_locs::vertices), &vertices, sizeof(VertexData3D) * 4);

	cellGcmSetupContextData(&Gcm, ptr_caste(addr + mem_locs::fifo, u32), 0x60000, GcmCallback);
	gcmLabel displaySync = c.newLabel();
	gcmLabel start = c.newLabel();

	gcmLabel syncPointFirst = gcmLabel(0);

	for (u32 invalid_mask = 0, it = 0;; it++, invalid_mask = invalid_mask ? invalid_mask << 1 : 1)
	{
		if (it >= 33 * sizeof(valid_masks) / sizeof(u32))
		{
			break;
		}

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

		CellGcmSurface surface, surface_back;
		surface.type = CELL_GCM_SURFACE_PITCH;
		surface.antialias = CELL_GCM_SURFACE_CENTER_1;
		surface.colorFormat = CELL_GCM_SURFACE_A8R8G8B8;
		surface.colorTarget = CELL_GCM_SURFACE_TARGET_0;
		surface.colorLocation[0] = CELL_GCM_LOCATION_LOCAL;
		surface.colorOffset[0] = mem_locs::video;
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
		surface.depthOffset = mem_locs::depth;
		surface.depthPitch = 1280*2;
		surface.width = 1280;
		surface.height = 720;
		surface.x = 0;
		surface.y = 0;
		surface_back = surface;
		cellGcmSetSurface(&Gcm, &surface_back);

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
								CELL_GCM_TEXTURE_LINEAR,
								CELL_GCM_TEXTURE_LINEAR, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);

		CellGcmTexture tex = {};
		tex.location = CELL_GCM_LOCATION_LOCAL;
		tex.format = CELL_GCM_TEXTURE_A8R8G8B8 | CELL_GCM_TEXTURE_NR | CELL_GCM_TEXTURE_LN;
		tex.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
		tex.pitch = 1280*4;
		tex.height = 720;
		tex.width = 1280;
		tex.offset = mem_locs::texture;
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
			for (u32* fill = OffsetToLocal(mem_locs::texture); i < 1280 * 720; i++, fill++)
			{
				// Reset texture to white
				*fill = -1;
			}
		}

		{
			u32 i = 0;
			for (u32* fill = OffsetToLocal(mem_locs::video); i < 1280 * 720; i++, fill++)
			{
				// Black
				*fill = 0;
			}
		}

		cellGcmSetTexture(&Gcm, g_tex_sampler, &tex);

		// State setting
		cellGcmSetShadeMode(&Gcm, CELL_GCM_SMOOTH);

		cellGcmSetVertexProgram(&Gcm, vpFile, vpUcode);
		cellGcmSetFragmentProgram(&Gcm, fpFile, mem_locs::fragment);

		cellGcmSetVertexDataArray(&Gcm, g_obj_coord_idx, 0, sizeof(VertexData3D), 3, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, mem_locs::vertices );
		cellGcmSetVertexDataArray(&Gcm, g_tex_coord_idx, 0, sizeof(VertexData3D), 2, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, mem_locs::vertices + (sizeof(float) * 3) );

		cellGcmSetInvalidateTextureCache(&Gcm, CELL_GCM_INVALIDATE_TEXTURE);
		cellGcmSetInvalidateVertexCache(&Gcm);

		// Write all-mask
		cellGcmSetColorMask(&Gcm, CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B | CELL_GCM_COLOR_MASK_A);

		// Testing mask
		const u32 tested_mask = invalid_mask | valid_masks[it / 33];

		cellGcmSetColorMask(&Gcm, tested_mask);

		//cellGcmSetDrawInlineArray(&Gcm, CELL_GCM_PRIMITIVE_TRIANGLE_STRIP, sizeof(vertices) / 4, &vertices);
		cellGcmSetDrawArrays(&Gcm, CELL_GCM_PRIMITIVE_TRIANGLE_STRIP, 0, sizeof(vertices) / 4);
		cellGcmSetWriteBackEndLabel(&Gcm, 64, -2u);
		cellGcmSetWaitLabel(&Gcm, 64, -2u);
		cellGcmSetWriteBackEndLabel(&Gcm, 64, 0);

		surface.colorOffset[0] = mem_locs::video;
		surface_back = surface;
		cellGcmSetSurface(&Gcm, &surface_back);
		tex.location = CELL_GCM_LOCATION_LOCAL;
		tex.format = CELL_GCM_TEXTURE_A8R8G8B8 | CELL_GCM_TEXTURE_NR | CELL_GCM_TEXTURE_LN;
		tex.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
		tex.pitch = 1280*4;
		tex.height = 720;
		tex.width = 1280;
		tex.offset = mem_locs::texture;
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
		cellGcmSetTexture(&Gcm, g_tex_sampler, &tex);


		cellGcmSetInvalidateTextureCache(&Gcm, CELL_GCM_INVALIDATE_TEXTURE);
		cellGcmSetInvalidateVertexCache(&Gcm);

		//cellGcmSetDrawInlineArray(&Gcm, CELL_GCM_PRIMITIVE_TRIANGLE_STRIP, sizeof(vertices) / 4, &vertices);
		//ellGcmSetDrawArrays(&Gcm, CELL_GCM_PRIMITIVE_TRIANGLE_STRIP, 0, sizeof(vertices) / 4);
		cellGcmSetWriteBackEndLabel(&Gcm, 64, -2u);
		cellGcmSetWaitLabel(&Gcm, 64, -2u);
		cellGcmSetWriteBackEndLabel(&Gcm, 64, 0);

		cellGcmSetFlip(&Gcm, id);
		cellGcmSetWaitFlip(&Gcm);
		cellGcmSetReferenceCommand(&Gcm, 2);

		gcmLabel branchSync = c.newLabel();
		c.push(0); // Reserved for branch
		c.align_pad(32, true);

		gcmLabel syncPoint1 = c.newLabel();

		if (syncPointFirst.pos == 0)
		{
			syncPointFirst = syncPoint1;
		}
		else
		{
			// Ensure the same position has been revisited
			// align_pad should take care of slight mismatches in size (increase it if asserts)
			ENSURE_TRUE(syncPointFirst.pos == syncPoint1.pos);
		}

		c.jmp(syncPoint1);
		c.jmp(start);
		gcmLabel syncPoint2 = c.newLabel();
		c.jmp(syncPoint2);
		c.jmp(start);

		gcmLabel BranchToSelf = it % 2 == 0 ? syncPoint1 : syncPoint2;
		gcmLabel notBranchToSelf = it % 2 == 1 ? syncPoint1 : syncPoint2;

		*OffsetToAddr(branchSync.pos) = c.make_jmp(BranchToSelf); // Put branch to self stop point
		store_vol(*OffsetToAddr(notBranchToSelf.pos), 0); // Annul previous branch to self - freeing RSX!
		c.flush(); // Write PUT for the first kick
		c.set_write_pos(start.pos); // Reset command queue writer

		// Wait for RSX
		while (load_vol(ctrl->get) != BranchToSelf.pos)
		{
			sys_timer_usleep(1000);
		}

		printf("[%d] Used mask = 0x%08x\n Output Sample:\n", it, tested_mask);
		print_bytes(OffsetToLocal(mem_locs::video) + 32, 32);
	}

	printf("sample finished.\n");
	return 0;
}
