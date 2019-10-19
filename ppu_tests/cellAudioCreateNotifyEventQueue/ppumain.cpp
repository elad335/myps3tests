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

#include "../ppu_header.h"

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

	ENSURE_OK(cellAudioInit());

	// Test error code on invalid queue key
	cellFunc(AudioSetNotifyEventQueue, 0);

	u32 id;
	u64 key;
	ENSURE_OK(cellAudioCreateNotifyEventQueue(&id, &key));

	ENSURE_OK(cellAudioSetNotifyEventQueue(key));

	CellAudioPortParam param = {};
	param.nChannel = CELL_AUDIO_PORT_8CH;
	param.nBlock = CELL_AUDIO_BLOCK_16;
	param.attr = 0; 
	u32 port;
	cellAudioPortOpen(&param, &port);

	CellAudioPortConfig config = {};
	cellAudioGetPortConfig(port, &config);
	ENSURE_OK(cellAudioPortStart(port));

	sys_event_t event = {};
	sys_event_queue_receive(id, &event, 0);

	u64 tag;
	ENSURE_OK(cellAudioGetPortBlockTag(port, param.nBlock - 1, &tag));

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