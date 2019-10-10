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

namespace gLocations
{
	void* video = ptr_cast(0xC0000000); // Video
	void* fragment = ptr_cast(0xC0900000); // FP shader
	void* varray = ptr_cast(0xC0A00000); // Vertex array 1
	void* color1 = ptr_cast(0xC0D00000); // Color buffer 0
};

int main() {

	// They should be at the same address (Traps are not gay)
	if (int_cast(&Gcm) != int_cast(&c.c)) asm volatile ("tw 4, 1, 1");

	LoadModules();
	sys_memory_allocate(0x1000000, 0x400, &addr);

	cellGcmInit(1<<16, 0x100000, ptr_cast(addr)); 
	CellGcmControl* ctrl = cellGcmGetControlRegister();
	while (ctrl->put != ctrl->get) sys_timer_usleep(300);
	cellGcmMapEaIoAddress(ptr_cast(addr + (1<<20)), 1<<20, 15<<20);

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
		  // vertex           tex coord
		{ MINX,MINY,0.f,0,    0.f, 0.f },
		{ MAXX,MINY,0.f,0,    1.f, 0.f },
		{ MINX,MAXY,0.f,0,    0.f, 1.f },
		{ MAXX,MAXY,0.f,0,    1.f, 1.f }
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
		scale[1] = h * 0.5f;
		scale[2] = (max - min) * 0.5f;
		scale[3] = 0.0f;
		offset[0] = x + scale[0];
		offset[1] = y + h * 0.5f;
		offset[2] = (max + min) * 0.5f;
		offset[3] = 0.0f;

		cellGcmSetViewport(&Gcm, x, y, w, h, min, max, scale, offset);
		cellGcmSetScissor(&Gcm, x, y, w, h);
	}

	static const u32 colorOffset = 0x000000;
	static const u32 depthOffset = 0xF00000;
	static const u32 texOffset = 0x900000;

	{
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
		//cellGcmSetSurface(&Gcm, &surface);
	}

	//Get Cg Parameters
	{
		CGparameter objCoord =
			cellGcmCgGetNamedParameter( vpFile,
										"a2v.objCoord" );
		if (objCoord == 0)
			asm volatile ("tw 4, 1, 1");

		CGparameter texCoord =
			cellGcmCgGetNamedParameter( vpFile,
										"a2v.texCoord" );
		if (texCoord == 0)
			asm volatile ("tw 4, 1, 1");

		CGparameter texture =
			cellGcmCgGetNamedParameter( fpFile,
										"texture" );
		if (texture == 0)
			asm volatile ("tw 4, 1, 1");

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
		u32 i = 0, powers = 0;
		for (u32* fill = OffsetToAddr(texOffset); i < 1280 * 720; i++, fill++)
		{
			// Completely random colors
			*fill = (0xFF000000 | (powers += i));
		}
		fsync();
	}

	{
		u32 i = 0;
		for (u32* fill = (u32*)gLocations::video; i < 1280 * 720; i++, fill++)
		{
			*fill = -1u;
		}
		fsync();
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

	cellGcmSetInvalidateTextureCache(&Gcm, CELL_GCM_INVALIDATE_TEXTURE);
	cellGcmSetInvalidateVertexCache(&Gcm);

	//cellGcmSetDrawArrays(&Gcm, CELL_GCM_PRIMITIVE_TRIANGLE_STRIP, 0, 4);
	gfxFence(&Gcm);

	/*c.reg(NV3089_IMAGE_OUT_SIZE, (720 << 16) | 1280);
	c.reg(NV3089_IMAGE_IN_SIZE, (720 << 16) | 1280);
	c.reg(NV3089_IMAGE_IN_FORMAT, (CELL_GCM_TRANSFER_INTERPOLATOR_ZOH << 24) | (CELL_GCM_TRANSFER_ORIGIN_CORNER << 16) | (1280 * 4));
	c.reg(NV3089_SET_COLOR_FORMAT, CELL_GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8);
	c.reg(NV3089_IMAGE_OUT_POINT, 0); // x=0, y=0
	c.reg(NV3089_CLIP_POINT, 0);
	c.reg(NV3089_CLIP_SIZE, (720 << 16) | 1280);
	c.reg(NV3089_SET_OPERATION, CELL_GCM_TRANSFER_OPERATION_SRCCOPY);
	c.reg(NV3089_SET_CONTEXT_DMA_IMAGE, CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER);
	c.reg(NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN, CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER);
	c.reg(NV3089_SET_CONTEXT_SURFACE, CELL_GCM_CONTEXT_SURFACE2D);
	c.reg(NV3062_SET_COLOR_FORMAT, CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8);
	c.reg(NV3089_IMAGE_IN_OFFSET, colorOffset);
	c.reg(NV3062_SET_OFFSET_DESTIN, colorOffset);
	c.reg(NV3089_DS_DX, 1048576);
	c.reg(NV3089_DT_DY, 1048576);
	c.reg(NV3089_IMAGE_IN, 0);*/

	//cellGcmSetTileInfo(0, CELL_GCM_LOCATION_LOCAL, colorOffset, 1280*720*4, 64, CELL_GCM_COMPMODE_DISABLED, 0, 0);
	//cellGcmBindTile(0);
	cellGcmSetTransferScaleMode(&Gcm ,CELL_GCM_TRANSFER_MAIN_TO_LOCAL, CELL_GCM_TRANSFER_SURFACE);

	CellGcmTransferScale scale1 =
	{
		CELL_GCM_TRANSFER_OPERATION_SRCCOPY, // Unused
		CELL_GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8, // Format
		CELL_GCM_TRANSFER_OPERATION_SRCCOPY, // Operation
		0, 0, // Clip x, y
		1024, 720, // clip w, h
		0, 0, // out x, y
		1024, 720, // out w, h
		cellGcmGetFixedSint32(1.), cellGcmGetFixedSint32(1.), // Ratio w, h
		1024, 720, // in w, h
		1280*4, // Pitch 
		CELL_GCM_TRANSFER_ORIGIN_CENTER, // Origin
		CELL_GCM_TRANSFER_INTERPOLATOR_FOH, // Interp
		texOffset,
		0, 0 // in x, y
	};

	CellGcmTransferSurface surface1 =
	{
		CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8, // Format
		1280*4, // Pitch
		0, // Unused
		colorOffset
	};

	cellGcmSetTransferScaleSurface(&Gcm, &scale1, &surface1);
	//cellGcmSetTransferData(&Gcm, CELL_GCM_TRANSFER_MAIN_TO_LOCAL, colorOffset, 0, texOffset, 1280*4, 1280*4, 720);
	gfxFence(&Gcm);

	gcmLabel displaySync = c.newLabel();
	cellGcmSetFlip(&Gcm, id);
	cellGcmSetWaitFlip(&Gcm);
	c.jmp(displaySync);
	cellGcmSetReferenceCommand(&Gcm, 2);
	fsync();

	ctrl->put = c.pos();
	sys_timer_sleep(10);

	while (true) 
	{
		sys_timer_usleep(1000); 
	}

	printf("sample finished.\n");

	return 0;
}