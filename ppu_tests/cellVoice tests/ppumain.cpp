#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_syscache.h>
#include <cell/sysmodule.h>
#include <cell/camera.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/event.h>
#include <sys/memory.h>
#include <cell/gem.h>
#include <cell/camera.h>
#include <cell/voice.h>
#include <cell/spurs.h> 
#include <sys/spu_thread_group.h>
#include <sys/spu_thread.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>
#include <sys/event.h>
#include <sys/timer.h>
#include <sys/syscall.h>
#include <cell/gcm.h>
#include <cell/cell_fs.h>
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

sys_event_queue_t queue_id;

int main()
{
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_VOICE );

    CellVoiceInitParam Params;
    reset_obj(Params); 
    cellFunc(VoiceInit, &Params);

    cellFunc(VoiceStart);
	CellVoicePortParam PortArgs;
    PortArgs.portType                  = CELLVOICE_PORTTYPE_OUT_VOICE;
    PortArgs.bMute                     = false;
    PortArgs.threshold                 = 100;                   
    PortArgs.volume                    = 1.0f;
	store_vol(PortArgs.voice.bitrate, CELLVOICE_BITRATE_3850);

	u32 port = CELLVOICE_INVALID_PORT_ID;
	printf("Id tests:\n");

	for (u32 i = 0; i < 128; i++)
	{
		cellFunc(VoiceCreatePort, &port, &PortArgs);
		printf("portid = 0x%x\n", port);
		// Added bit is ignored
		cellFunc(VoiceDeletePort, port | (1u << 16));
		cellFunc(VoiceDeletePort, port);
		port = CELLVOICE_INVALID_PORT_ID;

		cellFunc(VoiceCreatePort, &port, &PortArgs);
		printf("portid = 0x%x\n", port);

		// This errors
		cellFunc(VoiceDeletePort, port & 0xff);
		cellFunc(VoiceDeletePort, port);
		port = CELLVOICE_INVALID_PORT_ID;
		sys_timer_usleep(5000);
	}

	if (true)
	{
		printf("Testing GetVolume() with invalid port id:\n");
		float volume = 0;
		cellFunc(VoiceGetVolume, 0xff, &volume);
		u32 volumeh = bit_cast<u32>(volume);
		printf("volume = %f (in hex = %x)\n", volume, volumeh);
	}

	if (true)
	{
		printf("Testing GetBitRate() with IN_MIC port type:\n");
    	PortArgs.portType                  = CELLVOICE_PORTTYPE_IN_PCMAUDIO;
    	PortArgs.bMute                     = false;
    	PortArgs.threshold                 = 100;                   
    	PortArgs.volume                    = 1.0f;
		reset_obj(PortArgs.device.playerId, 0);
		port = CELLVOICE_INVALID_PORT_ID;

		cellFunc(VoiceCreatePort, &port, &PortArgs);
		u32 bitrate = 0;
		cellFunc(VoiceGetBitRate, port, &bitrate);
		cellFunc(VoiceDeletePort, port);
		printf("bitrate = 0x%x\n", bitrate);
	}

	printf("Testing Voice start/stop():\n");
	cellFunc(VoiceStart); cellFunc(VoiceStart);
	cellFunc(VoiceStop); cellFunc(VoiceStop);

	if (true)
	{
		printf("Testing Voice[Create,Set,Remove]NotifyEventQueue()\n");
		cellFunc(VoiceSetNotifyEventQueue, 0, 0);
		u64 key = ~0ull;

		sys_event_queue_attribute_t attr;
		attr.attr_protocol = SYS_SYNC_FIFO;
		attr.type = SYS_PPU_QUEUE;
		reset_obj(attr.name);

		// Find an empty queue key, create it and destroy
		for (;; key--)
		{
			if (const s32 res = sys_event_queue_create(&queue_id, &attr, key, 0x40))
			{
				if (res != EEXIST)
				{
					printf("Error creating event queue! 0x%x\n", res);
					return res;
				}
			}
			else
			{
				break;
			}
		}

		ENSURE_OK(sys_event_queue_destroy(queue_id, 0));

		cellFunc(VoiceSetNotifyEventQueue, key, 0);

		ENSURE_OK(cellVoiceCreateNotifyEventQueue(&queue_id, &key));
		ENSURE_OK(cellVoiceSetNotifyEventQueue(key, 1));

		// Try to enqueue the key twice (with different source)
		cellFunc(VoiceSetNotifyEventQueue, key, 2);

		// Send an event
		cellFunc(VoiceStart);

		sys_event_t event;
		reset_obj(event);
		ENSURE_OK(sys_event_queue_receive(queue_id, &event, 0));
		printf("Voice event's source: 0x%llx\n", event.source);

		sys_timer_sleep(1);

		// Recieves two events with different source!
		reset_obj(event);
		ENSURE_OK(sys_event_queue_receive(queue_id, &event, 0));
		printf("Second Voice event's source: 0x%llx\n", event.source);

		// Test if another event is sent (with timeout)
		cellFunc(VoiceStart);
		sys_timer_sleep(1);
		g_ec = sys_event_queue_receive(queue_id, &event, 1000000);
		printf("sys_event_queue_receive(error=0x%x)\n", (u32)g_ec);
	}

	printf("sample finished.\n");
	return 0;
}