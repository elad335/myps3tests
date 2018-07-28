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

	// Map twice, into different effective addresses
	cellGcmMapEaIoAddress(ptr_cast(addr), 0x0, 0x100000);
	cellGcmMapEaIoAddress(ptr_cast(addr + (1<<20)), 0x0, 0x100000);

	CellGcmControl* ctrl = cellGcmGetControlRegister();

	// Wait for RSX to complete previous operation
	do sys_timer_usleep(200); while (ctrl->get != ctrl->put);

	u32* compiler = ptr_caste(addr, u32);

	sys_timer_usleep(40);
	{
		// Place a jump into io address 0

		const u32 get = ctrl->get;

		if (get >= (1<<20))
		{
			printf("unexpected FIFO get ptr: 0x%x aborting", get);
			return -1;
		}
	
		compiler[get / 4] = 0 | RSX_METHOD_NEW_JUMP_CMD;
		compiler[(1<<18) + (get / 4)] = 0 | RSX_METHOD_NEW_JUMP_CMD;
	}
	sys_timer_usleep(40);

	// test which command gets executed
	// REF = 0x12345678: the io offset's EA hasn't changed by the second map
	// REF = 0x23456789: the io offset's EA got changed by the second map

	compiler[1] = 0x12345678; // Value for ref cmd
	compiler[2] = 0x8 | RSX_METHOD_NEW_JUMP_CMD; // branch to self

	compiler[(1<<18) + 1] = 0x23456789;
	compiler[(1<<18) + 2] = 0x8 | RSX_METHOD_NEW_JUMP_CMD; // branch to self
	asm volatile ("eieio;sync");

	compiler[0] = 0x00040050; // NV406E_SET_REFERENCE
	compiler[(1<<18) + 0] = 0x00040050; // NV406E_SET_REFERENCE

	sys_timer_usleep(100);
	ctrl->put = 0x100000;
	sys_timer_usleep(100);

	while(ctrl->ref == -1u) sys_timer_usleep(200);

	printf("REF=0x%x, GET=0x%x\n", ctrl->ref, ctrl->get);
	asm volatile("eieio;sync");

	// The final test: test how the RSX manages io translation internally
	// Crash: multiple mapping are not happening, unmapping and mapping overwrites previous maps
	// No crash: the RSX saves "mapping points", when unmapping it "pops" the firss entry, multiple maps are happening
	cellGcmUnmapIoAddress(0);
	while(1) sys_timer_usleep(4000);

    return 0;
}