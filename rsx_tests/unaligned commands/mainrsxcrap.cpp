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
};

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;
sys_addr_t addr;

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

	CellGcmControl* ctrl = cellGcmGetControlRegister();

	// Wait for RSX to complete previous operation
	do sys_timer_usleep(200); while (ctrl->get != ctrl->put);

	c.begin = 0;
	c.current = 0;

	// Place a jump into io address 0
	c.set_cmd(ctrl->get, 0 | RSX_METHOD_NEW_JUMP_CMD);
	sys_timer_usleep(40);

	c.push(0x40053); // unaligned NV406E_SET_REFERENCE cmd
	c.push(0x1234); // Value for ref cmd
	asm volatile ("sync;eieio");

	ctrl->put = 0x8;
	sys_timer_usleep(1000);

	// Wait for complition
	while(ctrl->get != 8) sys_timer_usleep(100);

	printf("sample finished. REF=0x%x", ctrl->ref);

    return 0;
}