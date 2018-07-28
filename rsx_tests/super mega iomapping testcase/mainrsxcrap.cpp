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

struct RsxDmaControl
{
	u8 resv[0x40];
	u32 put;
	u32 get;
	u32 ref;
	u32 unk[2];
	u32 unk1;
};

// Table for translation, as we dont have the gcm's
struct RsxIoAddrTable
{
	u16 ea[0x200];
} RSXMem;

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;
sys_addr_t addr1;
sys_addr_t addr2;
sys_event_queue_t queue_id;

int main() {

	register int ret asm ("3");
	{
		u64 addr;
		system_call_3(SYS_RSX_DEVICE_MAP, int_cast(&addr), 0, 8);

		printf("ret is 0x%x, addr = 0x%x\n", ret, addr);
	}

	u32 mem_handle;
	u64 mem_addr; 
	{
		system_call_7(SYS_RSX_MEMORY_ALLOCATE, int_cast(&mem_handle), int_cast(&mem_addr), 0xf900000, 0x80000, 0x300000, 0xf, 0x8); //size = 0xf900000, 249 mb

		printf("ret is 0x%x, mem_handle=0x%x, mem_addr=0x%x\n", ret, mem_handle, mem_addr);
	}

	u32 context_id;
	u64 lpar_dma_control, lpar_driver_info, lpar_reports;
	{
		system_call_6(SYS_RSX_CONTEXT_ALLOCATE, int_cast(&context_id), int_cast(&lpar_dma_control), int_cast(&lpar_driver_info), int_cast(&lpar_reports), mem_handle, 0x820);

		printf("ret is 0x%x, context_id=0x%x, lpar_dma_control=0x%x, lpar_driver_info=0x%x, lpar_reports=0x%x\n", ret, context_id, lpar_dma_control, lpar_driver_info, lpar_reports);
	}

	sys_mmapper_allocate_memory(0x800000, SYS_MEMORY_GRANULARITY_1M, &mem_id);

	printf("ret is 0x%x, mem_id=0x%x\n", ret, mem_id);

	sys_mmapper_allocate_address(0x10000000, 0x40f, 0x10000000, &addr1);

	printf("ret is 0x%x, addr1=0x%x\n", ret, addr1);

	sys_mmapper_map_memory(u64(addr1), mem_id, SYS_MEMORY_PROT_READ_WRITE);

	printf("ret is 0x%x\n", ret);

	{
		// Test error code returned while unmapping already unmapped memory
		system_call_3(SYS_RSX_CONTEXT_IOUNMAP, context_id, 0x000000, 0x00200000 /*size = 2mb*/);
		printf("sys_rsx_context_iounmap1: ret is 0x%x\n", ret);
	}

	// Test if a second map overwrites the previous

	{
		system_call_5(SYS_RSX_CONTEXT_IOMAP, context_id, 0x000000, u64(addr1), 0x00200000 /*size = 2mb*/, 0xe000000000000800ull);
		printf("sys_rsx_context_iomap1: ret is 0x%x\n", ret);
	}

	{
		RSXMem.ea[0] = addr1 >> 20;
		system_call_5(SYS_RSX_CONTEXT_IOMAP, context_id, 0x000000, u64(addr1) + (1<<20), 0x00200000 /*size = 2mb*/, 0xe000000000000800ull);
		printf("sys_rsx_context_iomap1: ret is 0x%x\n", ret);
	}

	{
		// FIFO? unconfirmed
		system_call_6(SYS_RSX_CONTEXT_ATTRIBUTE, context_id, 0x1, 0x1000, 0x1000, 0x0, 0x0);
		printf("fifo: ret is 0x%x\n", ret);
	}

	RsxDmaControl* ctrl = ptr_caste(lpar_dma_control, RsxDmaControl);
	//ctrl->ref = 0; // Reset ref register
	//sys_timer_usleep(40);

	u32* compiler = ptr_caste(addr1, u32);

	// (test which command get executed)

	/*compiler[0] = NV406E_SET_REFERENCE | (1 << RSX_METHOD_NON_INCREMENT_COUNT_SHIFT) | RSX_METHOD_NON_INCREMENT_CMD;
	compiler[1] = 0x123; // Value for ref cmd
	compiler[2] = 0x8 | RSX_METHOD_NEW_JUMP_CMD; // branch to self

	compiler[(0x100000 >> 2) + 0] = NV406E_SET_REFERENCE | (1 << RSX_METHOD_NON_INCREMENT_COUNT_SHIFT) | RSX_METHOD_NON_INCREMENT_CMD;
	compiler[(0x100000 >> 2) + 1] = 0x234;
	compiler[(0x100000 >> 2) + 2] = 0x8 | RSX_METHOD_NEW_JUMP_CMD; */// branch to self

	{
		// Place a jump into io address 0
		const u32 get = ctrl->get;
		sys_timer_usleep(40);
		printf("get=0x%x", get);
		*ptr_caste(((u32)RSXMem.ea[get >> 20] << 20) | (get & 0xFFFFF), u32) = 0 | RSX_METHOD_NEW_JUMP_CMD;
	}

	sys_timer_usleep(100);
	ctrl->put = 0x100000;
	sys_timer_usleep(100);

	while(!ctrl->ref) sys_timer_usleep(100);
	sys_timer_usleep(100);
	printf("ref=0x%x\n", ctrl->ref);
	
    printf("sample finished.\n");

    return 0;
}