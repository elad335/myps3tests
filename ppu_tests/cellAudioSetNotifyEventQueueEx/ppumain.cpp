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
#include <sys/sys_time.h> 

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;

sys_addr_t addr1;
sys_addr_t addr2;

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_AUDIO );

	ENSURE_OK(cellAudioInit());

	u32 id;
	u64 key;
	ENSURE_OK(cellAudioCreateNotifyEventQueue(&id, &key));
	s64 sec[2], nsec[2], count = 0;
	cellFunc(AudioSetNotifyEventQueueEx, key, CELL_AUDIO_EVENTFLAG_DECIMATE_4 | CELL_AUDIO_EVENTFLAG_DECIMATE_2);

	sys_event_t event[1];
	ENSURE_OK(sys_event_queue_receive(id, event, 0));
	sys_time_get_current_time(&sec[0], &nsec[0]);

	for (; count < 100; count++)
	{
		ENSURE_OK(sys_event_queue_receive(id, event, 0));
	}

	sys_time_get_current_time(&sec[1], &nsec[1]);
	s64 diff = 0;
	diff = (sec[1] - sec[0]) * 1000000000;
	if (nsec[1] < nsec[0]) diff -= nsec[0] - nsec[1];
	else diff += nsec[1] - nsec[0];
	printf("passed=%lld\n", diff / 100);

	cellFunc(AudioRemoveNotifyEventQueue, key);
	cellFunc(AudioRemoveNotifyEventQueueEx, key, CELL_AUDIO_EVENTFLAG_DECIMATE_4 | CELL_AUDIO_EVENTFLAG_DECIMATE_2);

	return 0;
}