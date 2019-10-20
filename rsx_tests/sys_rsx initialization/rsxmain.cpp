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

#include "../rsx_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack	: 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

// use static to avoid allocations, even though its usually goes into the stack
static sys_memory_t mem_id;
static u64 dev_addr;
static u32 mem_handle;
static u64 mem_addr;

struct RsxDriverInfo
{
	u32 version_driver;     // 0x0
	u32 version_gpu;        // 0x4
	u32 memory_size;        // 0x8
	u32 hardware_channel;   // 0xC
	u32 nvcore_frequency;   // 0x10
	u32 memory_frequency;   // 0x14
	u32 unk1[4];            // 0x18 - 0x24
	u32 unk2;               // 0x28 -- pgraph stuff
	u32 reportsNotifyOffset;// 0x2C offset to notify memory
	u32 reportsOffset;      // 0x30 offset to reports memory
	u32 reportsReportOffset;// 0x34 offset to reports in reports memory
	u32 unk3[6];            // 0x38-0x54
	u32 systemModeFlags;    // 0x54
	u8 unk4[0x1064];              // 0x10B8

	struct Head
	{
		u64 lastFlipTime;    // 0x0 last flip time
		u32 flipFlags;       // 0x8 flags to handle flip/queue
		u32 unk1;            // 0xC
		u32 flipBufferId;    // 0x10
		u32 queuedBufferId;  // 0x14 todo: this is definately not this variable but its 'unused' so im using it for queueId to pass to flip handler
		u32 unk3;            // 0x18
		u32 unk6;            // 0x18 possible low bits of time stamp?  used in getlastVBlankTime
		u64 lastSecondVTime; // 0x20 last time for second vhandler freq
		u64 unk4;            // 0x28
		u64 vBlankCount;     // 0x30
		u32 unk;             // 0x38 possible u32, 'flip field', top/bottom for interlaced
		u32 unk5;            // 0x3C possible high bits of time stamp? used in getlastVBlankTime
	} head[8]; // size = 0x40, 0x200

	u32 unk7;          // 0x12B8
	u32 unk8;          // 0x12BC
	u32 handlers;      // 0x12C0 -- flags showing which handlers are set
	u32 unk9;          // 0x12C4
	u32 unk10;         // 0x12C8
	u32 userCmdParam;  // 0x12CC
	u32 handler_queue; // 0x12D0
	u32 unk11;         // 0x12D4
	u32 unk12;         // 0x12D8
	u32 unk13;         // 0x12DC
	u32 unk14;         // 0x12E0
	u32 unk15;         // 0x12E4
	u32 unk16;         // 0x12E8
	u32 unk17;         // 0x12F0
	u32 lastError;     // 0x12F4 error param for cellGcmSetGraphicsHandler
							 // todo: theres more to this
};

u32 sys_rsx_memory_allocate(u32* mem_handle, u64* mem_addr)
{
	// Note: memory_size set to  0xf200000 as passed from sdk version 1.9 <= x < 2.2
	system_call_7(SYS_RSX_MEMORY_ALLOCATE, int_cast(mem_handle), int_cast(mem_addr), 0xf200000, 0x80000, 0x300000, 0xf, 0x8);
	return_to_user_prog(u32);
}
u32 sys_rsx_context_allocate(u32* context_id, u64* lpar_dma_control, u64* lpar_driver_info, u64* lpar_reports, u32 mem_handle)
{
	system_call_6(SYS_RSX_CONTEXT_ALLOCATE, int_cast(context_id), int_cast(lpar_dma_control), int_cast(lpar_driver_info), int_cast(lpar_reports), mem_handle /*0x5a5a5a5b*/, 0x820);
	return_to_user_prog(u32);
}

u32 sys_rsx_device_map(u64* dev_addr)
{
	system_call_3(SYS_RSX_DEVICE_MAP, int_cast(dev_addr), 0, 8);
	return_to_user_prog(u32);
}

int main() {

	// Ensure 1mb allocation memory block is allocated so it wont mess with printf
	{u32 dummy; ENSURE_OK(sys_memory_allocate(0x1000000, 0x400, &dummy)); }

	u32 context_id;
	u64 lpar_dma_control, lpar_driver_info, lpar_reports;
	printf("sys_rsx_context_allocate before sys_rsx_memory_allocate returned: 0x%x\n", sys_rsx_context_allocate(&context_id, &lpar_dma_control, &lpar_driver_info, &lpar_reports, mem_handle));

	if (sys_rsx_memory_allocate(&mem_handle, &mem_addr) != CELL_OK) return 0;
	{u32 dummy; u64 dummy2; printf("sys_rsx_memory_allocate second call error code: 0x%x\n", sys_rsx_memory_allocate(&dummy, &dummy2));}

	sys_rsx_context_allocate(&context_id, &lpar_dma_control, &lpar_driver_info, &lpar_reports, mem_handle);

	sys_rsx_device_map(&dev_addr);
	{u64 dummy; printf("sys_rsx_device_map second call returned: 0x%x\n", sys_rsx_device_map(&dev_addr));}

	printf("dev_addr=0x%x, lpar_dma_control=0x%llx, lpar_driver_info=0x%llx, lpar_reports=0x%llx, memory_size=0x%x\n", 
	dev_addr, lpar_dma_control, lpar_driver_info, lpar_reports, ptr_caste(lpar_driver_info, RsxDriverInfo)->memory_size);
	printf("sample finished.\n");

	return 0;
}