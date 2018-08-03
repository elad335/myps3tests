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
#include <memory>

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define int_cast(addr) reinterpret_cast<uintptr_t>(addr)
#define ptr_cast(intnum) reinterpret_cast<void*>(intnum)
#define ptr_caste(intnum, type) reinterpret_cast<type*>(ptr_cast(intnum))

enum
{
	RSX_METHOD_OLD_JUMP_CMD_MASK = 0xe0000003,
	RSX_METHOD_OLD_JUMP_CMD = 0x20000000,
	RSX_METHOD_OLD_JUMP_OFFSET_MASK = 0x1ffffffc,

	RSX_METHOD_INCREMENT_CMD_MASK = 0xe0030003,
	RSX_METHOD_INCREMENT_CMD = 0,
	RSX_METHOD_INCREMENT_COUNT_MASK = 0x0ffc0000,
	RSX_METHOD_INCREMENT_COUNT_SHIFT = 18,
	RSX_METHOD_INCREMENT_METHOD_MASK = 0x00001ffc,

	RSX_METHOD_NON_INCREMENT_CMD_MASK = 0xe0030003,
	RSX_METHOD_NON_INCREMENT_CMD = 0x40000000,
	RSX_METHOD_NON_INCREMENT_COUNT_MASK = 0x0ffc0000,
	RSX_METHOD_NON_INCREMENT_COUNT_SHIFT = 18,
	RSX_METHOD_NON_INCREMENT_METHOD_MASK = 0x00001ffc,

	RSX_METHOD_NEW_JUMP_CMD_MASK = 0x00000003,
	RSX_METHOD_NEW_JUMP_CMD = 0x00000001,
	RSX_METHOD_NEW_JUMP_OFFSET_MASK = 0xfffffffc,

	RSX_METHOD_CALL_CMD_MASK = 0x00000003,
	RSX_METHOD_CALL_CMD = 0x00000002,
	RSX_METHOD_CALL_OFFSET_MASK = 0xfffffffc,

	RSX_METHOD_RETURN_CMD = 0x00020000,
};

enum
{
	// NV40_CHANNEL_DMA (NV406E)
	NV406E_SET_REFERENCE = 0x00000050 >> 2,
	NV406E_SET_CONTEXT_DMA_SEMAPHORE = 0x00000060 >> 2,
	NV406E_SEMAPHORE_OFFSET = 0x00000064 >> 2,
	NV406E_SEMAPHORE_ACQUIRE = 0x00000068 >> 2,
	NV406E_SEMAPHORE_RELEASE = 0x0000006c >> 2,

	// NV03_MEMORY_TO_MEMORY_FORMAT	(NV0039)
	NV0039_SET_OBJECT = 0x00002000 >> 2,
	NV0039_SET_CONTEXT_DMA_NOTIFIES = 0x00002180 >> 2,
	NV0039_SET_CONTEXT_DMA_BUFFER_IN = 0x00002184 >> 2,
	NV0039_SET_CONTEXT_DMA_BUFFER_OUT = 0x00002188 >> 2,
	NV0039_OFFSET_IN = 0x0000230C >> 2,
	NV0039_OFFSET_OUT = 0x00002310 >> 2,
	NV0039_PITCH_IN = 0x00002314 >> 2,
	NV0039_PITCH_OUT = 0x00002318 >> 2,
	NV0039_LINE_LENGTH_IN = 0x0000231C >> 2,
	NV0039_LINE_COUNT = 0x00002320 >> 2,
	NV0039_FORMAT = 0x00002324 >> 2,
	NV0039_BUFFER_NOTIFY = 0x00002328 >> 2,

	// NV30_IMAGE_FROM_CPU (NV308A)
	NV308A_SET_OBJECT = 0x0000A000 >> 2,
	NV308A_SET_CONTEXT_DMA_NOTIFIES = 0x0000A180 >> 2,
	NV308A_SET_CONTEXT_COLOR_KEY = 0x0000A184 >> 2,
	NV308A_SET_CONTEXT_CLIP_RECTANGLE = 0x0000A188 >> 2,
	NV308A_SET_CONTEXT_PATTERN = 0x0000A18C >> 2,
	NV308A_SET_CONTEXT_ROP = 0x0000A190 >> 2,
	NV308A_SET_CONTEXT_BETA1 = 0x0000A194 >> 2,
	NV308A_SET_CONTEXT_BETA4 = 0x0000A198 >> 2,
	NV308A_SET_CONTEXT_SURFACE = 0x0000A19C >> 2,
	NV308A_SET_COLOR_CONVERSION = 0x0000A2F8 >> 2,
	NV308A_SET_OPERATION = 0x0000A2FC >> 2,
	NV308A_SET_COLOR_FORMAT = 0x0000A300 >> 2,
	NV308A_POINT = 0x0000A304 >> 2,
	NV308A_SIZE_OUT = 0x0000A308 >> 2,
	NV308A_SIZE_IN = 0x0000A30C >> 2,
	NV308A_COLOR = 0x0000A400 >> 2,
};

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;
sys_addr_t addr;
uint32_t g_width;
uint32_t g_height;
uint32_t g_color_pitch;
uint32_t g_depth_pitch;
uint32_t g_color_offs[BUF_NUM];
uint32_t g_depth_offs;
uint32_t g_frame_idx;
/*E vertex structure. */
struct my_vertex {
	float px, py, pz;
	float tx, ty;
} *g_vertex_buf;
uint32_t g_vertex_buf_offs;
uint32_t g_obj_coord_idx;
uint32_t g_tex_coord_idx;
float Total_Time;
CGresource g_sampler;
CGprogram g_vertex_prg;
CGprogram g_fragment_prg;
CellGcmTexture g_tex_param;
void* g_vertex_prg_ucode;
void* g_fragment_prg_ucode;

struct rsxCommandCompiler
{
public:
	u32 begin;
	u32 current;

	inline void set_cmd(u32 addr, u32 cmd)
	{
		u32* ptr;
		cellGcmIoOffsetToAddress(begin + addr, ptr_caste(&ptr, void*));
		*ptr = cmd;
	}

	inline void push(u32 cmd)
	{
		u32* ptr;
		cellGcmIoOffsetToAddress(current, ptr_caste(&ptr, void*));
		*ptr = cmd;
		current += 4;
	}

};

static rsxCommandCompiler c;

int main() {

	register int ret asm ("3");

	if (cellSysmoduleIsLoaded(CELL_SYSMODULE_GCM_SYS) == CELL_SYSMODULE_ERROR_UNLOADED) 
	{
	   cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );
	}

	sys_mmapper_allocate_memory(0x800000, SYS_MEMORY_GRANULARITY_1M, &mem_id);

	printf("ret is 0x%x, mem_id=0x%x\n", ret, mem_id);

	sys_mmapper_allocate_address(0x10000000, 0x40f, 0x10000000, &addr);

	printf("ret is 0x%x, addr=0x%x\n", ret, addr);

	sys_mmapper_map_memory(u64(addr), mem_id, SYS_MEMORY_PROT_READ_WRITE);

	printf("ret is 0x%x\n", ret);

	cellGcmInit(1<<16, 0x100000, ptr_cast(addr));
	cellGcmMapEaIoAddress(ptr_cast(addr + (1<<20)), 0x100000, 0x100000);

	CellGcmControl* ctrl = cellGcmGetControlRegister();

	// Wait for RSX to complete previous operation
	do sys_timer_usleep(200); while (ctrl->get != ctrl->put);

	c.begin = 0;
	c.current = 0;

	// Place a jump into io address 0
	c.set_cmd(ctrl->get, 0 | RSX_METHOD_NEW_JUMP_CMD);
	sys_timer_usleep(40);

	c.push(4 | RSX_METHOD_CALL_CMD); // call-to-next
	c.push(8 | RSX_METHOD_CALL_CMD); // Call-to-next
	c.push(12 | RSX_METHOD_NEW_JUMP_CMD); // Branch-to-self
	asm volatile ("sync;eieio");

	ctrl->put = 12;
	sys_timer_usleep(1000);

	// Wait for complition
	while(ctrl->get != 12) sys_timer_usleep(100);

	printf("sample finished.");

    return 0;
}

void drawTex()
{
	void *gcm_buf;
	void *depth_addr;
	void *color_base_addr;
	void *color_addr[BUF_NUM];

	uint32_t color_size;
	uint32_t color_limit;
	uint32_t depth_size;
	uint32_t depth_limit;
	uint32_t buffer_width;
	
	CellVideoOutState state;
	CellVideoOutResolution resolution;

	/*E load libgcm_sys */
	prx_ret = cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );
	if( prx_ret < CELL_OK ){
		fprintf( stderr, "disp_create> cellSysmoduleLoadModule failed ... %#x\n", prx_ret );
	}

	sys_ret = cellVideoOutGetState( CELL_VIDEO_OUT_PRIMARY, 0, &state );
	if( sys_ret != CELL_OK ){
		fprintf( stderr, "disp_create> cellVideoOutGetState failed ... %#x\n", sys_ret );
	}

	sys_ret = cellVideoOutGetResolution( state.displayMode.resolutionId, &resolution );
	if( sys_ret != CELL_OK ){
		fprintf( stderr, "disp_create> cellVideoOutGetResolution failed ... %#x\n", sys_ret );
	}

	g_width = resolution.width;
	g_height = resolution.height;

	printf("Display Width: %d Height: %d\n",g_width,g_height);

	buffer_width = ROUNDUP( g_width, 64 );
	gcm_buf = memalign( 0x100000, 0x100000 );

	/*E initialize the gcm. */
	gcm_ret = cellGcmInit( 0x100000 - 0x10000, 0x100000, gcm_buf );

	if( CELL_OK != gcm_ret ){
		fprintf( stderr, "disp_create> cellGcmInit failed ... %d\n", gcm_ret );	
	}

	g_color_pitch =
		cellGcmGetTiledPitchSize( buffer_width*4 );
	if( 0 == g_color_pitch ){
		fprintf( stderr, "disp_create> cellGcmGetTiledPitchSize failed ...\n" );	
	}
	
	g_depth_pitch =
		cellGcmGetTiledPitchSize( buffer_width*4 );
	if( 0 == g_depth_pitch ){
		fprintf( stderr, "disp_create> cellGcmGetTiledPitchSize failed ...\n" );	
	}
	
	/*E color buffer settings. */
	color_size = g_color_pitch*ROUNDUP( g_height, 64 );
	color_limit = ROUNDUP( BUF_NUM*color_size,
						   0x10000 );
	
	/*E depth buffer settings. */
	depth_size = g_depth_pitch*ROUNDUP( g_height, 64 );
	depth_limit = ROUNDUP( depth_size,
						   0x10000 );

	CellVideoOutConfiguration config;
	memset( &config, 0, sizeof(CellVideoOutConfiguration) );
	config.pitch = g_color_pitch;
	config.resolutionId = state.displayMode.resolutionId;
	config.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;

	sys_ret = cellVideoOutConfigure( CELL_VIDEO_OUT_PRIMARY, &config, NULL, 0 );
	if( CELL_OK != sys_ret ){
		fprintf( stderr, "disp_create> cellVideoOutConfigure failed ... %d\n", sys_ret );
	}

	cellGcmSetFlipMode( CELL_GCM_DISPLAY_VSYNC );

	/*E get GPU configuration. */
	CellGcmConfig gcm_cfg;
	cellGcmGetConfiguration( &gcm_cfg );
	g_vram_heap = (uintptr_t)gcm_cfg.localAddress;
	
	/*E allocate color buffers. */
	_vheap_align( 0x10000 );
	color_base_addr = _vheap_alloc( color_limit );

	for( i_cnt=0; i_cnt<BUF_NUM; i_cnt++ ){
		color_addr[i_cnt] = (void*)
			((uintptr_t)color_base_addr + (i_cnt*color_size));
		cellGcmAddressToOffset( color_addr[i_cnt],
								&g_color_offs[i_cnt] );
	}
	
	/*E allocate depth buffers. */
	_vheap_align( 0x10000 );

	depth_addr = _vheap_alloc( depth_limit );

	cellGcmAddressToOffset( depth_addr,
							&g_depth_offs );
	
	/*E set tiled region. */
	cellGcmSetTile( 0, CELL_GCM_LOCATION_LOCAL,
					g_color_offs[0], color_limit, g_color_pitch,
					CELL_GCM_COMPMODE_DISABLED, 0, 0 );
	
	cellGcmSetTile( 1, CELL_GCM_LOCATION_LOCAL,
					g_depth_offs, depth_limit, g_depth_pitch,
					CELL_GCM_COMPMODE_DISABLED, 0, 0 );
	
	cellGcmSetZcull( 0, g_depth_offs, buffer_width,
					 ROUNDUP(g_height, 64),
					 0, CELL_GCM_ZCULL_Z24S8,
					 CELL_GCM_SURFACE_CENTER_1,
					 CELL_GCM_LESS, CELL_GCM_ZCULL_LONES,
					 CELL_GCM_LESS, 0x80, 0xff );
	
	/*E regist surface */
	for( i_cnt=0; i_cnt<BUF_NUM; i_cnt++ ){
		gcm_ret = cellGcmSetDisplayBuffer( i_cnt,
										   g_color_offs[i_cnt],
										   g_color_pitch,
										   g_width,
										   g_height );
		if( CELL_OK != gcm_ret ) return;
	}
	
	/*E set initial target */
	set_render_tgt( 0 );

	/*E load shaders. */
	init_shader();

	_vheap_align( 512*1024 );

	/*E create vertex buffer & send vertex data to vram. */
	my_vertex vertices[4] = {
		/*E vertex           tex coord */
		{ -1.f, -1.f, 0.f,  0.f, 1.f },
		{  1.f, -1.f, 0.f,  1.f, 1.f },
		{ -1.f,  1.f, 0.f,  0.f, 0.f },
		{  1.f,  1.f, 0.f,  1.f, 0.f }
	};

	/*E create small vertex buffer & send small vertex data to vram. */
	g_vertex_buf = (my_vertex*)_vheap_alloc( sizeof(my_vertex)*4 );

	memcpy( g_vertex_buf, vertices, sizeof(my_vertex)*4 );

	cellGcmAddressToOffset( (void*)g_vertex_buf,
							&g_vertex_buf_offs );

	/*E tex parameter */
	g_tex_param.format  = CELL_GCM_TEXTURE_A8R8G8B8;
	/*E if it's not swizzle. */
	g_tex_param.format |= CELL_GCM_TEXTURE_LN;

	g_tex_param.remap =
		CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
		CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
		CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
		CELL_GCM_TEXTURE_REMAP_REMAP <<  8 |
		CELL_GCM_TEXTURE_REMAP_FROM_G << 6 |
		CELL_GCM_TEXTURE_REMAP_FROM_R << 4 |
		CELL_GCM_TEXTURE_REMAP_FROM_A << 2 |
		CELL_GCM_TEXTURE_REMAP_FROM_B;

	g_tex_param.mipmap = 1;
	g_tex_param.cubemap = CELL_GCM_FALSE;
	g_tex_param.dimension = CELL_GCM_TEXTURE_DIMENSION_2;

	/*E Get Cg Parameters */
	CGparameter objCoord =
		cellGcmCgGetNamedParameter( g_vertex_prg,
									"a2v.objCoord" );
	if( 0 == objCoord ){
		fprintf( stderr, "disp_create> cellGcmCgGetNamedParameter... fail\n" );
		return;
	}

	CGparameter texCoord =
		cellGcmCgGetNamedParameter( g_vertex_prg,
									"a2v.texCoord" );
	if( 0 == texCoord ){
		fprintf( stderr, "disp_create> cellGcmCgGetNamedParameter... fail\n" );
		return;
	}

	CGparameter texture =
		cellGcmCgGetNamedParameter( g_fragment_prg,
									"texture" );
	if( 0 == texture ){
		fprintf( stderr, "disp_create> cellGcmCgGetNamedParameter... fail\n" );
		return;
	}
	
	/*E Get attribute index */
	g_obj_coord_idx =
		( cellGcmCgGetParameterResource( g_vertex_prg,
										 objCoord) - CG_ATTR0 );
	g_tex_coord_idx =
		( cellGcmCgGetParameterResource( g_vertex_prg,
										 texCoord) - CG_ATTR0 );
	g_sampler = (CGresource)
		( cellGcmCgGetParameterResource( g_fragment_prg,
										 texture ) - CG_TEXUNIT0 );
}