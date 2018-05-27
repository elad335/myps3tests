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
#include <functional>

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t memory_id;
sys_addr_t addr1;
sys_addr_t addr2;

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int32_t s32;

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

void threadEntry(uint64_t queueId) {
	sys_event_t event;
	int ret = sys_event_queue_receive((sys_event_queue_t)queueId, &event, 0);
	while (ret == 0) {
		if (event.data2 != 2) // ignore vblank
			printf("event: 0x%llx 0x%llx 0x%llx 0x%llx\n", event.source, event.data1, event.data2, event.data3);
		
		ret = sys_event_queue_receive((sys_event_queue_t)queueId, &event, 0);
	}
	
	printf("threadended\n");
}

int main() {

    {
        u64 addr;
		u64 a2;
        system_call_3(SYS_RSX_DEVICE_MAP,  (uint64_t)(&addr), (uint64_t)&a2, 8);
		int ret = (int)(p1);
        printf("ret is 0x%x, addr = 0x%llx\n", ret, addr);
    }
	u32 mem_handle;
	{
		u64 mem_addr;
		system_call_7(SYS_RSX_MEMORY_ALLOCATE, (uintptr_t)(&mem_handle), (uintptr_t)(&mem_addr), 0xf900000, 0x80000, 0x300000, 0xf, 0x8); //size = 0xf900000, 249 mb
		int ret = (int)(p1);
		printf("ret is 0x%x, mem_handle= 0x%x, mem_addr= 0x%llx\n", ret, mem_handle, mem_addr);
	}

	u32 context_id;
	u64 lpar_dma_control;
	u64 lpar_driver_info;
	u64 lpar_reports;
	{
		system_call_6(SYS_RSX_CONTEXT_ALLOCATE, (uintptr_t)(&context_id), (uintptr_t)(&lpar_dma_control), (uintptr_t)(&lpar_driver_info), (uintptr_t)(&lpar_reports), mem_handle, 0x820);
		int ret = (int)(p1);
		printf("ret is 0x%x\n", ret);
	}
	
	RsxDriverInfo* driverInfo;
	driverInfo = (RsxDriverInfo*)(s32)lpar_driver_info;
	
	printf("handlerQueue 0x%x\n", driverInfo->handler_queue);
	
	int result;
	sys_ppu_thread_t m_tid1;
	// start thread 
	{
		
		result = sys_ppu_thread_create(
			&m_tid1,
			threadEntry,
			driverInfo->handler_queue,
			1002, 0x10000,
			SYS_PPU_THREAD_CREATE_JOINABLE,
			"test");
		if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
		}
	}	

	sys_memory_t mem_id[0x60];

	printf("allocaddr\n");
	result = sys_mmapper_allocate_address(0x10000000, 0x40f, 0x10000000, &addr1);
	if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
    }

	for (u64 i=0; i < 0x60; i++) { 

	printf("allocmem\n");
	result = sys_mmapper_allocate_memory(0x100000, SYS_MEMORY_GRANULARITY_1M, &mem_id[i]);
	if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
    }

	printf("mapmem\n");
	result = sys_mmapper_map_memory(u64(addr1) + (i<<20), mem_id[i], SYS_MEMORY_PROT_READ_WRITE);
	if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
    }
	}
	printf("iomap\n");
	{
		u64 flags = 0x00000800;
		flags |= (u64)0xf0000000 << 32;
		system_call_5(SYS_RSX_CONTEXT_IOMAP, context_id, 0 << 20, u64(addr1), 0x59 << 20, flags);
		int ret = (int)(p1);
		printf("ret is 0x%x\n", ret);
	}
	sys_timer_sleep(1);
	
	printf("unmapmem\n");
	result = sys_mmapper_unmap_memory(u64(addr1) + (31<<20), &mem_id[31]);
	if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
    }
	sys_timer_sleep(1);
	
	printf("allocaddr2\n");
	result = sys_mmapper_allocate_address(0x10000000, 0x40f, 0x10000000, &addr1);
	if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
    }
	sys_timer_sleep(1);
	
	uint64_t exit_status;
	printf("join thread\n");
	
	sys_ppu_thread_join(m_tid1, &exit_status);
	
    printf("sample finished.\n");

    return 0;
}