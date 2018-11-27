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

inline void mfence() { asm volatile ("sync;eieio"); };
inline void __check() { asm volatile ("twi 0x10, 3, 0"); };

#define SYS_APP_HOME "/app_home"

#define VP_PROGRAM SYS_APP_HOME "/mainvp.vpo"
#define FP_PROGRAM SYS_APP_HOME "/mainfp.fpo"

enum
{
	FP_SHADER_LOCATION_LOCAL = 1,
	FP_SHADER_LOCATION_MAIN = 2,
};

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
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

static union
{
	CellGcmContextData Gcm;
	rsxCommandCompiler c;
};

// CellGcmContextCallback
int GcmCallback(struct CellGcmContextData *, uint32_t){}

namespace loc
{
	enum
	{
		video = 0,
		fragment,
		vertex,
		varray,
		color1,

		__enum_max,
	};
};

void* gLocations[static_cast<size_t>(loc::__enum_max)] =
{
	ptr_cast(0xC0200000), // Video
	ptr_cast(0xC0900000), // FP shader
	NULL, // VP shader
	ptr_cast(0xC0A00000), // Vertex array
	ptr_cast(0xC0B00000), // Color buffer 1

};

struct Vertex4D{float i[4];};
static Vertex4D Varray[1] = 
{
	{.1, .2, .3, .4}
};

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

	// Load Shaders binaries and configs
	{
		u32 size = readFile(VP_PROGRAM,&vpFile);
		if (!size) asm volatile ("tw 4, 1, 1");

		cellCgbRead(vpFile, size, &vp), __check();

		//retrieve the vertex hardware configuration
		cellCgbGetVertexConfiguration(&vp,&vpConf);

		vpUcodeSize = cellCgbGetUCodeSize(&vp);

		memcpy(gLocations[loc::vertex] = ptr_cast(addr + (1<<15)), cellCgbGetUCode(&vp), vpUcodeSize);
	}

	{
		u32 size = readFile(FP_PROGRAM,&fpFile);
		if (!size) asm volatile ("tw 4, 1, 1");

		cellCgbRead(fpFile, size, &fp), __check();

		//retrieve the fragment program configuration
		cellCgbGetFragmentConfiguration(&fp,&fpConf);

		fpUcodeSize = cellCgbGetUCodeSize(&fp);

		memcpy(gLocations[loc::fragment], cellCgbGetUCode(&fp), fpUcodeSize);
	}

	// Initial vertex array
	memcpy(gLocations[loc::varray], &Varray, sizeof(Vertex4D));

	cellGcmSetupContextData(&Gcm, ptr_caste(addr + (1<<20), u32), 0x10000, GcmCallback);

	static const u32 colorOffset = 0x400000;
	static const u32 depthOffset = 0x140000;

	CellGcmSurface surface;
	surface.type = CELL_GCM_SURFACE_PITCH;
	surface.antialias = CELL_GCM_SURFACE_CENTER_1;
	surface.colorFormat = CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8;
	surface.colorTarget = CELL_GCM_SURFACE_TARGET_0;
	for (u32 i = 0, pitch = cellGcmGetTiledPitchSize(1280*4); i < 4; i++)
	{
		surface.colorLocation[i] = CELL_GCM_LOCATION_MAIN;
		surface.colorOffset[i] = colorOffset;
		surface.colorPitch[i] = pitch;
	}
	surface.depthFormat = CELL_GCM_SURFACE_Z16;
	surface.depthLocation = CELL_GCM_LOCATION_MAIN;
	surface.depthOffset = depthOffset;
	surface.depthPitch = 64;
	surface.width = 1280;
	surface.height = 720;
	surface.x = 0;
	surface.y = 0;
	cellGcmSetSurface(&Gcm, &surface);

	cellGcmSetVertexProgramLoad(&Gcm, &vpConf, gLocations[loc::vertex]);
	fpConf.offset = 0x00900000;
	cellGcmSetFragmentProgramLoad(&Gcm, &fpConf);
	cellGcmSetVertexDataArray(&Gcm, 0, 1, 4 * sizeof(float), 2, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0x0A00001);

	// By default the address shown in crash dump is 0
	// but thats because the location is always decremented by 1 to match the CellGcm constants CELL_GCM_LOCATION_MAIN and CELL_GCM_LOCATION_LOCAL
	// e.g Pushing value 0x2 (offset = 0, location = main) shows address 1 in the crash dump 
	c.push(1, NV4097_SET_SHADER_PROGRAM);
	c.push(0 | FP_SHADER_LOCATION_MAIN); // Offset = 0, Location = MAIN

	cellGcmSetDrawArrays(&Gcm, CELL_GCM_PRIMITIVE_POINTS, 0, 1);
	c.debugBreak();

	cellGcmSetReferenceCommand(&Gcm, 2);
	mfence();

	ctrl->put = c.newLabel().pos;
	sys_timer_usleep(100);

	while (ctrl->ref != 2) sys_timer_usleep(1000);

	printf("sample finished.\n");

    return 0;
}