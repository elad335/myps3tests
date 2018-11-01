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
#include <cell/gcm.h>
#include <cell/cgb.h>
#include <cell/resc.h>
#include <Cg/NV/cg.h>
#include <Cg/cgc.h>
#include <memory>
#include <stdint.h>
#include <string.h>

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define int_cast(addr) reinterpret_cast<uintptr_t>(addr)
#define ptr_cast(intnum) reinterpret_cast<void*>(intnum)
#define ptr_caste(intnum, type) reinterpret_cast<type*>(ptr_cast(intnum))
inline void mfence() { asm volatile ("sync;eieio"); };
inline void __check() { asm volatile ("twi 0x10, 3, 0"); };

 // _binary_{SHADERFILENAME}_f/vpo_start loads the shader
extern char _binary_mainvp_vpo_start[];
extern char _binary_mainfp_fpo_start[];

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
	NV406E_SET_REFERENCE = 0x00000050,
	NV406E_SET_CONTEXT_DMA_SEMAPHORE = 0x00000060,
	NV406E_SEMAPHORE_OFFSET = 0x00000064,
	NV406E_SEMAPHORE_ACQUIRE = 0x00000068,
	NV406E_SEMAPHORE_RELEASE = 0x0000006c,

	// NV03_MEMORY_TO_MEMORY_FORMAT	(NV0039)
	NV0039_SET_OBJECT = 0x00002000,
	NV0039_SET_CONTEXT_DMA_NOTIFIES = 0x00002180,
	NV0039_SET_CONTEXT_DMA_BUFFER_IN = 0x00002184,
	NV0039_SET_CONTEXT_DMA_BUFFER_OUT = 0x00002188,
	NV0039_OFFSET_IN = 0x0000230C,
	NV0039_OFFSET_OUT = 0x00002310,
	NV0039_PITCH_IN = 0x00002314,
	NV0039_PITCH_OUT = 0x00002318,
	NV0039_LINE_LENGTH_IN = 0x0000231C,
	NV0039_LINE_COUNT = 0x00002320,
	NV0039_FORMAT = 0x00002324,
	NV0039_BUFFER_NOTIFY = 0x00002328,

	// NV30_IMAGE_FROM_CPU (NV308A)
	NV308A_SET_OBJECT = 0x0000A000,
	NV308A_SET_CONTEXT_DMA_NOTIFIES = 0x0000A180,
	NV308A_SET_CONTEXT_COLOR_KEY = 0x0000A184,
	NV308A_SET_CONTEXT_CLIP_RECTANGLE = 0x0000A188,
	NV308A_SET_CONTEXT_PATTERN = 0x0000A18C,
	NV308A_SET_CONTEXT_ROP = 0x0000A190,
	NV308A_SET_CONTEXT_BETA1 = 0x0000A194,
	NV308A_SET_CONTEXT_BETA4 = 0x0000A198,
	NV308A_SET_CONTEXT_SURFACE = 0x0000A19C,
	NV308A_SET_COLOR_CONVERSION = 0x0000A2F8,
	NV308A_SET_OPERATION = 0x0000A2FC,
	NV308A_SET_COLOR_FORMAT = 0x0000A300,
	NV308A_POINT = 0x0000A304,
	NV308A_SIZE_OUT = 0x0000A308,
	NV308A_SIZE_IN = 0x0000A30C,
	NV308A_COLOR = 0x0000A400,

	// NV40_CURIE_PRIMITIVE	(NV4097)
	NV4097_SET_OBJECT = 0x00000000,
	NV4097_NO_OPERATION = 0x00000100,
	NV4097_NOTIFY = 0x00000104,
	NV4097_WAIT_FOR_IDLE = 0x00000110,
	NV4097_PM_TRIGGER = 0x00000140,
	NV4097_SET_CONTEXT_DMA_NOTIFIES = 0x00000180,
	NV4097_SET_CONTEXT_DMA_A = 0x00000184,
	NV4097_SET_CONTEXT_DMA_B = 0x00000188,
	NV4097_SET_CONTEXT_DMA_COLOR_B = 0x0000018c,
	NV4097_SET_CONTEXT_DMA_STATE = 0x00000190,
	NV4097_SET_CONTEXT_DMA_COLOR_A = 0x00000194,
	NV4097_SET_CONTEXT_DMA_ZETA = 0x00000198,
	NV4097_SET_CONTEXT_DMA_VERTEX_A = 0x0000019c,
	NV4097_SET_CONTEXT_DMA_VERTEX_B = 0x000001a0,
	NV4097_SET_CONTEXT_DMA_SEMAPHORE = 0x000001a4,
	NV4097_SET_CONTEXT_DMA_REPORT = 0x000001a8,
	NV4097_SET_CONTEXT_DMA_CLIP_ID = 0x000001ac,
	NV4097_SET_CONTEXT_DMA_CULL_DATA = 0x000001b0,
	NV4097_SET_CONTEXT_DMA_COLOR_C = 0x000001b4,
	NV4097_SET_CONTEXT_DMA_COLOR_D = 0x000001b8,
	NV4097_SET_SURFACE_CLIP_HORIZONTAL = 0x00000200,
	NV4097_SET_SURFACE_CLIP_VERTICAL = 0x00000204,
	NV4097_SET_SURFACE_FORMAT = 0x00000208,
	NV4097_SET_SURFACE_PITCH_A = 0x0000020c,
	NV4097_SET_SURFACE_COLOR_AOFFSET = 0x00000210,
	NV4097_SET_SURFACE_ZETA_OFFSET = 0x00000214,
	NV4097_SET_SURFACE_COLOR_BOFFSET = 0x00000218,
	NV4097_SET_SURFACE_PITCH_B = 0x0000021c,
	NV4097_SET_SURFACE_COLOR_TARGET = 0x00000220,
	NV4097_SET_SURFACE_PITCH_Z = 0x0000022c,
	NV4097_INVALIDATE_ZCULL = 0x00000234,
	NV4097_SET_CYLINDRICAL_WRAP = 0x00000238,
	NV4097_SET_CYLINDRICAL_WRAP1 = 0x0000023c,
	NV4097_SET_SURFACE_PITCH_C = 0x00000280,
	NV4097_SET_SURFACE_PITCH_D = 0x00000284,
	NV4097_SET_SURFACE_COLOR_COFFSET = 0x00000288,
	NV4097_SET_SURFACE_COLOR_DOFFSET = 0x0000028c,
	NV4097_SET_WINDOW_OFFSET = 0x000002b8,
	NV4097_SET_WINDOW_CLIP_TYPE = 0x000002bc,
	NV4097_SET_WINDOW_CLIP_HORIZONTAL = 0x000002c0,
	NV4097_SET_WINDOW_CLIP_VERTICAL = 0x000002c4,
	NV4097_SET_DITHER_ENABLE = 0x00000300,
	NV4097_SET_ALPHA_TEST_ENABLE = 0x00000304,
	NV4097_SET_ALPHA_FUNC = 0x00000308,
	NV4097_SET_ALPHA_REF = 0x0000030c,
	NV4097_SET_BLEND_ENABLE = 0x00000310,
	NV4097_SET_BLEND_FUNC_SFACTOR = 0x00000314,
	NV4097_SET_BLEND_FUNC_DFACTOR = 0x00000318,
	NV4097_SET_BLEND_COLOR = 0x0000031c,
	NV4097_SET_BLEND_EQUATION = 0x00000320,
	NV4097_SET_COLOR_MASK = 0x00000324,
	NV4097_SET_STENCIL_TEST_ENABLE = 0x00000328,
	NV4097_SET_STENCIL_MASK = 0x0000032c,
	NV4097_SET_STENCIL_FUNC = 0x00000330,
	NV4097_SET_STENCIL_FUNC_REF = 0x00000334,
	NV4097_SET_STENCIL_FUNC_MASK = 0x00000338,
	NV4097_SET_STENCIL_OP_FAIL = 0x0000033c,
	NV4097_SET_STENCIL_OP_ZFAIL = 0x00000340,
	NV4097_SET_STENCIL_OP_ZPASS = 0x00000344,
	NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE = 0x00000348,
	NV4097_SET_BACK_STENCIL_MASK = 0x0000034c,
	NV4097_SET_BACK_STENCIL_FUNC = 0x00000350,
	NV4097_SET_BACK_STENCIL_FUNC_REF = 0x00000354,
	NV4097_SET_BACK_STENCIL_FUNC_MASK = 0x00000358,
	NV4097_SET_BACK_STENCIL_OP_FAIL = 0x0000035c,
	NV4097_SET_BACK_STENCIL_OP_ZFAIL = 0x00000360,
	NV4097_SET_BACK_STENCIL_OP_ZPASS = 0x00000364,
	NV4097_SET_SHADE_MODE = 0x00000368,
	NV4097_SET_BLEND_ENABLE_MRT = 0x0000036c,
	NV4097_SET_COLOR_MASK_MRT = 0x00000370,
	NV4097_SET_LOGIC_OP_ENABLE = 0x00000374,
	NV4097_SET_LOGIC_OP = 0x00000378,
	NV4097_SET_BLEND_COLOR2 = 0x0000037c,
	NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE = 0x00000380,
	NV4097_SET_DEPTH_BOUNDS_MIN = 0x00000384,
	NV4097_SET_DEPTH_BOUNDS_MAX = 0x00000388,
	NV4097_SET_CLIP_MIN = 0x00000394,
	NV4097_SET_CLIP_MAX = 0x00000398,
	NV4097_SET_CONTROL0 = 0x000003b0,
	NV4097_SET_LINE_WIDTH = 0x000003b8,
	NV4097_SET_LINE_SMOOTH_ENABLE = 0x000003bc,
	NV4097_SET_ANISO_SPREAD = 0x000003c0,
	NV4097_SET_SCISSOR_HORIZONTAL = 0x000008c0,
	NV4097_SET_SCISSOR_VERTICAL = 0x000008c4,
	NV4097_SET_FOG_MODE = 0x000008cc,
	NV4097_SET_FOG_PARAMS = 0x000008d0,
	NV4097_SET_SHADER_PROGRAM = 0x000008e4,
	NV4097_SET_VERTEX_TEXTURE_OFFSET = 0x00000900,
	NV4097_SET_VERTEX_TEXTURE_FORMAT = 0x00000904,
	NV4097_SET_VERTEX_TEXTURE_ADDRESS = 0x00000908,
	NV4097_SET_VERTEX_TEXTURE_CONTROL0 = 0x0000090c,
	NV4097_SET_VERTEX_TEXTURE_CONTROL3 = 0x00000910,
	NV4097_SET_VERTEX_TEXTURE_FILTER = 0x00000914,
	NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT = 0x00000918,
	NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR = 0x0000091c,
	NV4097_SET_VIEWPORT_HORIZONTAL = 0x00000a00,
	NV4097_SET_VIEWPORT_VERTICAL = 0x00000a04,
	NV4097_SET_POINT_CENTER_MODE = 0x00000a0c,
	NV4097_ZCULL_SYNC = 0x00000a1c,
	NV4097_SET_VIEWPORT_OFFSET = 0x00000a20,
	NV4097_SET_VIEWPORT_SCALE = 0x00000a30,
	NV4097_SET_POLY_OFFSET_POINT_ENABLE = 0x00000a60,
	NV4097_SET_POLY_OFFSET_LINE_ENABLE = 0x00000a64,
	NV4097_SET_POLY_OFFSET_FILL_ENABLE = 0x00000a68,
	NV4097_SET_DEPTH_FUNC = 0x00000a6c,
	NV4097_SET_DEPTH_MASK = 0x00000a70,
	NV4097_SET_DEPTH_TEST_ENABLE = 0x00000a74,
	NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR = 0x00000a78,
	NV4097_SET_POLYGON_OFFSET_BIAS = 0x00000a7c,
	NV4097_SET_VERTEX_DATA_SCALED4S_M = 0x00000a80,
	NV4097_SET_TEXTURE_CONTROL2 = 0x00000b00,
	NV4097_SET_TEX_COORD_CONTROL = 0x00000b40,
	NV4097_SET_TRANSFORM_PROGRAM = 0x00000b80,
	NV4097_SET_SPECULAR_ENABLE = 0x00001428,
	NV4097_SET_TWO_SIDE_LIGHT_EN = 0x0000142c,
	NV4097_CLEAR_ZCULL_SURFACE = 0x00001438,
	NV4097_SET_PERFORMANCE_PARAMS = 0x00001450,
	NV4097_SET_FLAT_SHADE_OP = 0x00001454,
	NV4097_SET_EDGE_FLAG = 0x0000145c,
	NV4097_SET_USER_CLIP_PLANE_CONTROL = 0x00001478,
	NV4097_SET_POLYGON_STIPPLE = 0x0000147c,
	NV4097_SET_POLYGON_STIPPLE_PATTERN = 0x00001480,
	NV4097_SET_VERTEX_DATA3F_M = 0x00001500,
	NV4097_SET_VERTEX_DATA_ARRAY_OFFSET = 0x00001680,
	NV4097_INVALIDATE_VERTEX_CACHE_FILE = 0x00001710,
	NV4097_INVALIDATE_VERTEX_FILE = 0x00001714,
	NV4097_PIPE_NOP = 0x00001718,
	NV4097_SET_VERTEX_DATA_BASE_OFFSET = 0x00001738,
	NV4097_SET_VERTEX_DATA_BASE_INDEX = 0x0000173c,
	NV4097_SET_VERTEX_DATA_ARRAY_FORMAT = 0x00001740,
	NV4097_CLEAR_REPORT_VALUE = 0x000017c8,
	NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE = 0x000017cc,
	NV4097_GET_REPORT = 0x00001800,
	NV4097_SET_ZCULL_STATS_ENABLE = 0x00001804,
	NV4097_SET_BEGIN_END = 0x00001808,
	NV4097_ARRAY_ELEMENT16 = 0x0000180c,
	NV4097_ARRAY_ELEMENT32 = 0x00001810,
	NV4097_DRAW_ARRAYS = 0x00001814,
	NV4097_INLINE_ARRAY = 0x00001818,
	NV4097_SET_INDEX_ARRAY_ADDRESS = 0x0000181c,
	NV4097_SET_INDEX_ARRAY_DMA = 0x00001820,
	NV4097_DRAW_INDEX_ARRAY = 0x00001824,
	NV4097_SET_FRONT_POLYGON_MODE = 0x00001828,
	NV4097_SET_BACK_POLYGON_MODE = 0x0000182c,
	NV4097_SET_CULL_FACE = 0x00001830,
	NV4097_SET_FRONT_FACE = 0x00001834,
	NV4097_SET_POLY_SMOOTH_ENABLE = 0x00001838,
	NV4097_SET_CULL_FACE_ENABLE = 0x0000183c,
	NV4097_SET_TEXTURE_CONTROL3 = 0x00001840,
	NV4097_SET_VERTEX_DATA2F_M = 0x00001880,
	NV4097_SET_VERTEX_DATA2S_M = 0x00001900,
	NV4097_SET_VERTEX_DATA4UB_M = 0x00001940,
	NV4097_SET_VERTEX_DATA4S_M = 0x00001980,
	NV4097_SET_TEXTURE_OFFSET = 0x00001a00,
	NV4097_SET_TEXTURE_FORMAT = 0x00001a04,
	NV4097_SET_TEXTURE_ADDRESS = 0x00001a08,
	NV4097_SET_TEXTURE_CONTROL0 = 0x00001a0c,
	NV4097_SET_TEXTURE_CONTROL1 = 0x00001a10,
	NV4097_SET_TEXTURE_FILTER = 0x00001a14,
	NV4097_SET_TEXTURE_IMAGE_RECT = 0x00001a18,
	NV4097_SET_TEXTURE_BORDER_COLOR = 0x00001a1c,
	NV4097_SET_VERTEX_DATA4F_M = 0x00001c00,
	NV4097_SET_COLOR_KEY_COLOR = 0x00001d00,
	NV4097_SET_SHADER_CONTROL = 0x00001d60,
	NV4097_SET_INDEXED_CONSTANT_READ_LIMITS = 0x00001d64,
	NV4097_SET_SEMAPHORE_OFFSET = 0x00001d6c,
	NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE = 0x00001d70,
	NV4097_TEXTURE_READ_SEMAPHORE_RELEASE = 0x00001d74,
	NV4097_SET_ZMIN_MAX_CONTROL = 0x00001d78,
	NV4097_SET_ANTI_ALIASING_CONTROL = 0x00001d7c,
	NV4097_SET_SURFACE_COMPRESSION = 0x00001d80,
	NV4097_SET_ZCULL_EN = 0x00001d84,
	NV4097_SET_SHADER_WINDOW = 0x00001d88,
	NV4097_SET_ZSTENCIL_CLEAR_VALUE = 0x00001d8c,
	NV4097_SET_COLOR_CLEAR_VALUE = 0x00001d90,
	NV4097_CLEAR_SURFACE = 0x00001d94,
	NV4097_SET_CLEAR_RECT_HORIZONTAL = 0x00001d98,
	NV4097_SET_CLEAR_RECT_VERTICAL = 0x00001d9c,
	NV4097_SET_CLIP_ID_TEST_ENABLE = 0x00001da4,
	NV4097_SET_RESTART_INDEX_ENABLE = 0x00001dac,
	NV4097_SET_RESTART_INDEX = 0x00001db0,
	NV4097_SET_LINE_STIPPLE = 0x00001db4,
	NV4097_SET_LINE_STIPPLE_PATTERN = 0x00001db8,
	NV4097_SET_VERTEX_DATA1F_M = 0x00001e40,
	NV4097_SET_TRANSFORM_EXECUTION_MODE = 0x00001e94,
	NV4097_SET_RENDER_ENABLE = 0x00001e98,
	NV4097_SET_TRANSFORM_PROGRAM_LOAD = 0x00001e9c,
	NV4097_SET_TRANSFORM_PROGRAM_START = 0x00001ea0,
	NV4097_SET_ZCULL_CONTROL0 = 0x00001ea4,
	NV4097_SET_ZCULL_CONTROL1 = 0x00001ea8,
	NV4097_SET_SCULL_CONTROL = 0x00001eac,
	NV4097_SET_POINT_SIZE = 0x00001ee0,
	NV4097_SET_POINT_PARAMS_ENABLE = 0x00001ee4,
	NV4097_SET_POINT_SPRITE_CONTROL = 0x00001ee8,
	NV4097_SET_TRANSFORM_TIMEOUT = 0x00001ef8,
	NV4097_SET_TRANSFORM_CONSTANT_LOAD = 0x00001efc,
	NV4097_SET_TRANSFORM_CONSTANT = 0x00001f00,
	NV4097_SET_FREQUENCY_DIVIDER_OPERATION = 0x00001fc0,
	NV4097_SET_ATTRIB_COLOR = 0x00001fc4,
	NV4097_SET_ATTRIB_TEX_COORD = 0x00001fc8,
	NV4097_SET_ATTRIB_TEX_COORD_EX = 0x00001fcc,
	NV4097_SET_ATTRIB_UCLIP0 = 0x00001fd0,
	NV4097_SET_ATTRIB_UCLIP1 = 0x00001fd4,
	NV4097_INVALIDATE_L2 = 0x00001fd8,
	NV4097_SET_REDUCE_DST_COLOR = 0x00001fe0,
	NV4097_SET_NO_PARANOID_TEXTURE_FETCHES = 0x00001fe8,
	NV4097_SET_SHADER_PACKER = 0x00001fec,
	NV4097_SET_VERTEX_ATTRIB_INPUT_MASK = 0x00001ff0,
	NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK = 0x00001ff4,
	NV4097_SET_TRANSFORM_BRANCH_BITS = 0x00001ff8,

	//lv1 hypervisor commands
	GCM_SET_DRIVER_OBJECT = 0x0000E000,
	GCM_FLIP_HEAD = 0x0000E920,          //0xE920:0xE924: Flip head 0 or 1
	GCM_DRIVER_QUEUE = 0x0000E940,       //0XE940:0xE95C: First two indices prepare display buffers, rest unknown
	GCM_SET_USER_COMMAND = 0x0000EB00,   //0xEB00:0xEB04: User interrupt

	GCM_FLIP_COMMAND = 0x0000FEAC,
};

#define CELL_GCM_CONTEXT_DMA_SEMAPHORE_RW 0x66606660
#define CELL_GCM_CONTEXT_DMA_SEMAPHORE_R 0x66616661
#define CELL_GCM_CONTEXT_DMA_DEVICE_RW 0x56616660
#define CELL_GCM_CONTEXT_DMA_DEVICE_R 0x56616661

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

static sys_memory_t mem_id;
static sys_addr_t addr;
static CellGcmOffsetTable offsetTable;

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

static int AddrToOffset(void* addr)
{
	return (offsetTable.ioAddress[int_cast(addr) >> 20] << 20) | (int_cast(addr) & (0xFFFFF));
}

static u32* OffsetToAddr(u32 offset)
{
	return ptr_caste(((offsetTable.eaAddress[offset >> 20] << 20) | (offset & 0xFFFFF)), u32);
}

static u32* LocalOffsetToAddr(u32 offset)
{
	return ptr_caste(offset + 0xC0000000, u32);
}

// Jump labels for FIFO queue
struct gcmLabel
{
	u32 pos;
	gcmLabel(){}
	gcmLabel(u32 pos){ this->pos = pos; }
};

// Command queue manager composed from an internal cellGcm compiler
// And helper functions for custom commands generation
struct rsxCommandCompiler
{
	CellGcmContextData c;

	// Push command
	void push(u32 cmd)
	{
		*(c.current++) = cmd;	
	}

	// Push command with count
	void push(u32 count, u32 command)
	{
		*(c.current++) = (count << 18) | command;	
	}

	// Push command with count 1, a simple write
	void write_reg(u32 command, u32 value)
	{
		*(c.current++) = (1u << 18) | command;
		*(c.current++) = value;
	}

	// Push a jump command, offset is specified by label
	void jmp(gcmLabel label)
	{
		*(c.current++) = label.pos | RSX_METHOD_NEW_JUMP_CMD;
	}

	void call(gcmLabel label)
	{
		*(c.current++) = label.pos | RSX_METHOD_CALL_CMD;
	}

	void ret()
	{
		*(c.current++) = RSX_METHOD_RETURN_CMD;
	}

	// generates a jump label by current position
	gcmLabel newLabel()
	{
		return gcmLabel(AddrToOffset(c.current));
	}

	// Get current position by offset
	u32 pos()
	{
		return AddrToOffset(c.current);
	}

	void debugBreak() 
	{ 
		*(c.current++) = 0xFFFFFFFCu | RSX_METHOD_NEW_JUMP_CMD;
	}
};

static union
{
	CellGcmContextData Gcm;
	rsxCommandCompiler c;
};

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
		u32 i = 0;
		for (u32* fill = OffsetToAddr(texOffset); i < 1280 * 720; i++, fill++)
		{
			//
			*fill = 0xFF000000u | i;
		}
		mfence();
	}

	{
		u32 i = 0;
		for (u32* fill = (u32*)gLocations::video; i < 1280 * 720; i++, fill++)
		{
			*fill = -1u;
		}
		mfence();
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

	cellGcmSetWriteBackEndLabel(&Gcm, 64, 0);
	cellGcmSetWaitLabel(&Gcm, 64, 0);
	cellGcmSetDrawArrays(&Gcm, CELL_GCM_PRIMITIVE_TRIANGLE_STRIP, 0, 4);

	c.write_reg(NV406E_SET_CONTEXT_DMA_SEMAPHORE, CELL_GCM_CONTEXT_DMA_SEMAPHORE_R);
	c.write_reg(NV4097_SET_CONTEXT_DMA_SEMAPHORE, CELL_GCM_CONTEXT_DMA_SEMAPHORE_RW); // Seems to be unused, gcm only sets default value to SEMAPHORE while resetting
	c.write_reg(NV4097_SET_SEMAPHORE_OFFSET, 64 << 4);
	c.write_reg(NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE, 0xfffeffff /* value is -2 */);

	// Ensure different locations
	c.write_reg(NV406E_SET_CONTEXT_DMA_SEMAPHORE, CELL_GCM_CONTEXT_DMA_DEVICE_R);
	c.write_reg(NV406E_SEMAPHORE_OFFSET, 64 << 4);
	c.write_reg(NV406E_SEMAPHORE_ACQUIRE, -2);

	cellGcmSetReferenceCommand(&Gcm, 3);
	cellGcmSetFlip(&Gcm, id);
	mfence();

	ctrl->put = c.newLabel().pos;
	sys_timer_usleep(100);

	while (ctrl->ref != 3) 
	{
		sys_timer_usleep(1000); 
	}

	printf("Rendering completed; acquire was successful\nsample finished.\n");

    return 0;
}