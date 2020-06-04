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


	u32 id;
	u64 key, key2;
	CellAudioPortParam param = {};
	param.nChannel = CELL_AUDIO_PORT_8CH;
	param.nBlock = CELL_AUDIO_BLOCK_16;
	param.attr = 0; 
	u32 port;
	ENSURE_OK(cellAudioPortOpen(&param, &port));

	ENSURE_OK(cellAudioPortStart(port));

	sys_event_t event = {};

	for (u32 i = 0; i < 2; i++)
	{
		ENSURE_OK(cellAudioCreateNotifyEventQueue(&id, &key));
		ENSURE_OK(cellAudioSetNotifyEventQueue(key));

		ENSURE_OK(sys_event_queue_receive(id, &event, 0));
		printf("event:\n");
		print_obj(event);

		ENSURE_OK(sys_event_queue_destroy(id, 0));
		ENSURE_OK(cellAudioCreateNotifyEventQueue(&id, &key2));
		ENSURE_OK(key != key2);

		if (i == 0)
		{

			sysCell(event_queue_receive, id, &event, 1000000);
			printf("event:\n");
			print_obj(event);
			cellFunc(AudioRemoveNotifyEventQueue, key2);
			ENSURE_OK(sys_event_queue_destroy(id, 0));
		}
		else if (i == 1)
		{
			cellFunc(AudioSetNotifyEventQueue, key);

			cellFunc(AudioRemoveNotifyEventQueue, key2);
			ENSURE_OK(sys_event_queue_destroy(id, 0));
		}
		else
		{
			
		}
		
		
	}


	printf("sample finished.\n");

	return 0;
}