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
	nullptr = 0,
	RSX_METHOD_NEW_JUMP_CMD = 0x1u,
	RSX_METHOD_RETURN_CMD = 0x00020000u,
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

	if (cellSysmoduleIsLoaded(CELL_SYSMODULE_GCM_SYS) == CELL_SYSMODULE_ERROR_UNLOADED) 
	{
	   cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );
	}

	sys_mmapper_allocate_memory(0x800000, SYS_MEMORY_GRANULARITY_1M, &mem_id);

	printf("ret is 0x%x, mem_id=0x%x\n", ret, mem_id);

	sys_mmapper_allocate_address(0x10000000, 0x40f, 0x10000000, &addr1);

	printf("ret is 0x%x, addr1=0x%x\n", ret, addr1);

	sys_mmapper_map_memory(u64(addr1), mem_id, SYS_MEMORY_PROT_READ_WRITE);

	printf("ret is 0x%x\n", ret);

	u32* gcm_compiler = ptr_caste(addr1, u32);

	cellGcmInit(1<<16, 0x100000, ptr_cast(gcm_compiler));

	cellGcmMapEaIoAddressWithFlags(gcm_compiler, 0x0, 0x100000, CELL_GCM_IOMAP_FLAG_STRICT_ORDERING);
	cellGcmMapEaIoAddressWithFlags(gcm_compiler, 0x100000, 0x100000, CELL_GCM_IOMAP_FLAG_STRICT_ORDERING);


	CellGcmControl* ctrl = cellGcmGetControlRegister();

	printf("ctrl=0x%x\n", int_cast(ctrl));

	// Wait for RSX to complete previous operation
	do sys_timer_usleep(200); while (ctrl->get != ctrl->put);

	sys_timer_usleep(40);
	void* getaddr;
	cellGcmIoOffsetToAddress(ctrl->get, &getaddr);
	*ptr_caste(getaddr, u32) = RSX_METHOD_RETURN_CMD;
	const u32 start = 0x100000;
	gcm_compiler[start + 0] = 0; // NOP
	gcm_compiler[start + 1] = 0;
	gcm_compiler[start + 2] = 0;
	gcm_compiler[start + 3] = 0;
	gcm_compiler[start + 4] = 0;
	gcm_compiler[start + 5] = (start + (4<<2)) | RSX_METHOD_NEW_JUMP_CMD; // Branch to SELF

	asm volatile ("eieio;sync");
	ctrl->get = start;
	ctrl->put = start + (4<<2);

	asm volatile ("eieio;sync");
	sys_timer_usleep(2000);
	
    printf("sample finished 0x%x.\n", ctrl->get);

    return 0;
}