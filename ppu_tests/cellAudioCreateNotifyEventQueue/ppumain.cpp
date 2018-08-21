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
#include <cell/cell_fs.h>
#include <cell/audio.h> 
#include <memory>
#include <iostream>
#include <string>
#include <list>
#include <map>

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define int_cast(addr) reinterpret_cast<uintptr_t>(addr)
#define ptr_cast(intnum) reinterpret_cast<void*>(intnum)

inline void trap_syscall() { asm volatile ("twi 0x10, 3, 0"); }; 

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;

sys_addr_t addr1;
sys_addr_t addr2;
sys_event_queue_t queue_id;

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_AUDIO );

	register int ret asm ("3");

	cellAudioInit();
	trap_syscall();

	// Test error code on invalid queue jey
    printf("cellAudioSetNotifyEventQueue: ret=0x%x \n", cellAudioSetNotifyEventQueue(0));

	u32 id;
	u64 key;
	cellAudioCreateNotifyEventQueue(&id, &key);
	trap_syscall();

	cellAudioSetNotifyEventQueue(key);
	trap_syscall();

	CellAudioPortParam param = {};
	param.nChannel = CELL_AUDIO_PORT_8CH;
	param.nBlock = CELL_AUDIO_BLOCK_16;
	param.attr = 0; 
	u32 port;
	cellAudioPortOpen(&param, &port);

	CellAudioPortConfig config = {};
	cellAudioGetPortConfig(port, &config);
	cellAudioPortStart(port);
	trap_syscall();

	sys_event_t event = {};
	sys_event_queue_receive(id, &event, 0);

	u64 tag;
	cellAudioGetPortBlockTag(port, param.nBlock - 1, &tag);
	trap_syscall();

	printf("data0=0x%x, data2=0x%x, data2=0x%x, data3=0x%x\n", event.source, event.data1, event.data2, event.data3);

	printf("port attributes: indexAddr=0x%llx, status=0x%x, nChannel=0x%llx, nBlock=0x%llx\n portSize=0x%x, portAddr=0x%x\n", 
	config.readIndexAddr,
	config.status,
	config.nChannel,
	config.nBlock,
	config.portSize,
	config.portAddr);

	printf("tag=0x%x", tag);

	printf("sample finished. ret=0%x\n", sys_event_queue_destroy(0, 0));

    return 0;
}