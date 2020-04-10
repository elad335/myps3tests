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

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack	: 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;
sys_addr_t addr;

static rsxCommandCompiler c;
static CellGcmContextData& Gcm = c.c;

// CellGcmContextCallback
int GcmCallback(struct CellGcmContextData *, uint32_t)
{

}


int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );

	g_ec = sys_mmapper_allocate_memory(0x800000, SYS_MEMORY_GRANULARITY_1M, &mem_id);

	printf("ret is 0x%x, mem_id=0x%x\n", g_ec, mem_id);

	g_ec = sys_mmapper_allocate_address(0x10000000, 0x40f, 0x10000000, &addr);

	printf("ret is 0x%x, addr=0x%x\n", g_ec, addr);

	g_ec = sys_mmapper_map_memory(u64(addr), mem_id, SYS_MEMORY_PROT_READ_WRITE);

	printf("ret is 0x%x\n", g_ec);

	CellVideoOutConfiguration resConfig;
	resConfig.pitch = 4*1280;
	resConfig.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
	resConfig.aspect = CELL_VIDEO_OUT_ASPECT_16_9;
	resConfig.resolutionId = CELL_VIDEO_OUT_RESOLUTION_720;
	reset_obj(resConfig.reserved);

	CellVideoOutOption option;
	cellVideoOutConfigure(0, &resConfig,&option,1);

	ENSURE_OK(cellGcmInit(1<<16, 0x100000, ptr_cast(addr)));
	GcmMapEaIoAddress(addr + (1<<20), 1<<20, 7<<20);
	u8 id;
	cellGcmGetCurrentDisplayBufferId(&id);

 	cellGcmSetDisplayBuffer(id, 0<<20, 1280*4, 1280, 720);
	printf("ret is 0x%x\n", g_ec);

	cellGcmGetOffsetTable(&offsetTable);

	const CellGcmDisplayInfo* disp_info = cellGcmGetDisplayInfo();
	printf("pitch=0x%x", disp_info->pitch);
	CellGcmControl* ctrl = cellGcmGetControlRegister();
	//memset(ptr_cast(0xC0000000), 0xf, 1<<18);
	// Wait for RSX to complete previous operation
	wait_for_fifo(ctrl);

	// Place a jump into io address 1mb
	*OffsetToAddr(ctrl->get) = (1<<20) | RSX_METHOD_NEW_JUMP_CMD;
	sys_timer_usleep(40);

	/*cellGcmSetupContextData(&Gcm, ptr_caste(addr + (1<<20), u32), 0x10000, GcmCallback);
	cellGcmSetBackPolygonMode(&Gcm, CELL_GCM_POLYGON_MODE_POINT);
	cellGcmSetCullFaceEnable(&Gcm, CELL_GCM_TRUE);
	cellGcmSetCullFaceEnable(&Gcm, -1); // Invalid boolean value
	cellGcmSetCullFace(&Gcm, CELL_GCM_FRONT); // Valid, non-default Cull Face value
	cellGcmSetCullFace(&Gcm, -1); // Invalid Cull Face

	static const u32 colorOffset = 0x40000;
	static const u32 depthOffset = 0x140000;

	CellGcmSurface surface;
	surface.type = CELL_GCM_SURFACE_PITCH;
	surface.antialias = CELL_GCM_SURFACE_CENTER_1;
	surface.colorFormat = CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8;
	surface.colorTarget = CELL_GCM_SURFACE_TARGET_NONE;
	for (u32 i = 0; i < 4; i++)
	{
		surface.colorLocation[i] = CELL_GCM_LOCATION_MAIN;
		surface.colorOffset[i] = colorOffset;
		surface.colorPitch[i] = 64;
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
	cellGcmSetDrawBegin(&Gcm, CELL_GCM_PRIMITIVE_POLYGON);
	// TODO: actually render polygons with the invalid cull face attribute
	cellGcmSetDrawEnd(&Gcm);
	cellGcmSetReferenceCommand(&Gcm, 0x1);
	cellGcmSetNopCommand(&Gcm, 1);*/

	cellGcmSetupContextData(&Gcm, ptr_caste(addr + (1<<20), u32), 0x10000, GcmCallback);
	
	c.push(1, NV4097_SET_CULL_FACE_ENABLE);
	c.push(CELL_GCM_TRUE);
	c.push(1, NV4097_SET_CULL_FACE);
	c.push(CELL_GCM_FRONT); // Valid, non-default cull face mode 
	c.push(1, NV4097_SET_CULL_FACE);
	c.push(-1); // Invalid cull face mode

	c.push(1, NV4097_SET_SURFACE_CLIP_HORIZONTAL);
	c.push(1280 << 16);
	c.push(1, NV4097_SET_SURFACE_CLIP_VERTICAL);
	c.push(720 << 16);

	c.push(1, NV4097_SET_BEGIN_END);
	c.push(CELL_GCM_PRIMITIVE_POLYGON);
	// TODO: actually render polygons with the invalid cull face attribute
	c.push(1, NV4097_SET_BEGIN_END);
	c.push(0);
	c.push(1, NV406E_SET_REFERENCE);
	c.push(1);
	c.push(c.pos() | RSX_METHOD_OLD_JUMP_CMD);
	fsync();

	ctrl->put = c.pos();
	sys_timer_usleep(100);

	// Wait for complition
	while(ctrl->ref != 1) sys_timer_usleep(20000);

	// Crash intentionally
	cellGcmUnmapIoAddress(0);
	cellGcmUnmapIoAddress(1<<20);
	ctrl->put = 0; 

	// Wait for crash dump to get registers state
	while(true) sys_timer_usleep(100000);

	printf("sample finished.");

	return 0;
}