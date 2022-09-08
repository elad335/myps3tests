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
#include <sysutil/sysutil_music2.h>
#include <sysutil/sysutil_music.h>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;

sys_addr_t addr1;
sys_addr_t addr2;
bool ok = false;

void loop_util()
{
	while (!ok)
	{
		sys_timer_usleep(100000);
		cellSysutilCheckCallback();
	}
	ok = false;
}

static void cb_musicutil(uint32_t event, void *param, void * /*userData*/)
{
	ok = true;
	switch(event){
	case CELL_MUSIC2_EVENT_STATUS_NOTIFICATION:
		if ( (uint32_t)param == CELL_MUSIC2_PB_STATUS_STOP ){
			printf("CELL_MUSIC2_EVENT_STATUS_NOTIFICATION CELL_MUSIC2_PB_STATUS_STOP\n");
		}
		if ( (uint32_t)param == CELL_MUSIC2_PB_STATUS_PLAY ){
			printf("CELL_MUSIC2_EVENT_STATUS_NOTIFICATION CELL_MUSIC2_PB_STATUS_PLAY\n");
		}
		if ( (uint32_t)param == CELL_MUSIC2_PB_STATUS_PAUSE ){
			printf("CELL_MUSIC2_EVENT_STATUS_NOTIFICATION CELL_MUSIC2_PB_STATUS_PAUSE\n");
		}
		if ( (uint32_t)param == CELL_MUSIC2_PB_STATUS_FASTFORWARD ){
			printf("CELL_MUSIC2_EVENT_STATUS_NOTIFICATION CELL_MUSIC2_PB_STATUS_FASTFORWARD\n");
		}
		if ( (uint32_t)param == CELL_MUSIC2_PB_STATUS_FASTREVERSE ){
			printf("CELL_MUSIC2_EVENT_STATUS_NOTIFICATION CELL_MUSIC2_PB_STATUS_FASTREVERSE\n");
		}
		break;

	case CELL_MUSIC2_EVENT_INITIALIZE_RESULT:
		if( (uint32_t)param != CELL_MUSIC2_OK ){
			printf("CELL_MUSIC2_EVENT_INITIALIZE_RESULT failed\n");
		}
		else {
			printf("CELL_MUSIC2_EVENT_INITIALIZE_RESULT success\n");
		}
		break;

	case CELL_MUSIC2_EVENT_FINALIZE_RESULT:
		if( (uint32_t)param != CELL_MUSIC2_OK ){
			printf("CELL_MUSIC2_EVENT_FINALIZE_RESULT failed\n");
		}
		else {
			printf("CELL_MUSIC2_EVENT_FINALIZE_RESULT success\n");
		}
		break;

	case CELL_MUSIC2_EVENT_SELECT_CONTENTS_RESULT:
		if( (uint32_t)param == CELL_MUSIC2_OK ){
			printf("CELL_MUSIC2_EVENT_SELECT_CONTENTS_RESULT success\n");
		}
		else if( (uint32_t)param == CELL_MUSIC2_CANCELED ){
			printf("CELL_MUSIC2_EVENT_SELECT_CONTENTS_RESULT canceled\n");
		}
		else {
			printf("CELL_MUSIC2_EVENT_SELECT_CONTENTS_RESULT failed\n");
		}
		break;

	case CELL_MUSIC2_EVENT_SET_PLAYBACK_COMMAND_RESULT:
		switch ( (int)param ) {
		case CELL_MUSIC2_OK:
			printf("CELL_MUSIC2_EVENT_SET_PLAYBACK_COMMAND_RESULT CELL_MUSIC2_OK\n");
			break;
		case CELL_MUSIC2_ERROR_GENERIC:
			printf("CELL_MUSIC2_EVENT_SET_PLAYBACK_COMMAND_RESULT CELL_MUSIC2_ERROR_GENERIC\n");
			break;
		case CELL_MUSIC2_ERROR_NO_MORE_CONTENT:
			printf("CELL_MUSIC2_EVENT_SET_PLAYBACK_COMMAND_RESULT CELL_MUSIC2_ERROR_NO_MORE_CONTENT\n");
			break;
		case CELL_MUSIC2_ERROR_BUSY:
			printf("CELL_MUSIC2_EVENT_SET_PLAYBACK_COMMAND_RESULT CELL_MUSIC2_ERROR_BUSY\n");
			break;
		case CELL_MUSIC2_PLAYBACK_FINISHED:
			printf("CELL_MUSIC2_EVENT_SET_PLAYBACK_COMMAND_RESULT CELL_MUSIC2_PLAYBACK_FINISHED\n");
			break;
		default:
			printf("CELL_MUSIC2_EVENT_SET_PLAYBACK_COMMAND_RESULT unknown param=[%x]\n", (int)param);
			break;
		}
		break;

	case CELL_MUSIC2_EVENT_SET_VOLUME_RESULT:
		if( (uint32_t)param != CELL_MUSIC2_OK ){
			printf("CELL_MUSIC2_EVENT_SET_VOLUME_RESULT failed\n");
		}
		else {
			printf("CELL_MUSIC2_EVENT_SET_VOLUME_RESULT success\n");
		}
		break;

	case CELL_MUSIC2_EVENT_SET_SELECTION_CONTEXT_RESULT:
		if( (uint32_t)param == CELL_MUSIC2_OK ){
			printf("CELL_MUSIC2_EVENT_SET_SELECTION_CONTEXT_RESULT CELL_MUSIC2_OK\n");
		} else if( (uint32_t)param == CELL_MUSIC2_CANCELED ){
			printf("CELL_MUSIC2_EVENT_SET_SELECTION_CONTEXT_RESULT CELL_MUSIC2_CANCELED\n");
		} else {
			printf("CELL_MUSIC2_EVENT_SET_SELECTION_CONTEXT_RESULT unknown param=[%x]\n", (uint32_t)param);
		}
		break;

	case CELL_MUSIC2_EVENT_UI_NOTIFICATION:
		if( (uint32_t)param == CELL_MUSIC2_DIALOG_OPEN ){
			printf("CELL_MUSIC2_EVENT_UI_NOTIFICATION CELL_MUSIC2_DIALOG_OPEN\n");
		} else if( (uint32_t)param == CELL_MUSIC2_DIALOG_CLOSE ){
			printf("CELL_MUSIC2_EVENT_UI_NOTIFICATION CELL_MUSIC2_DIALOG_CLOSE\n");
		} else {
			printf("CELL_MUSIC2_EVENT_UI_NOTIFICATION unknown param=[%x]\n", (uint32_t)param);
		}
		break;

	default:
		printf("cb_musicutil unknown event\n");
		break;
	}
}

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_MUSIC );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_MUSIC2 );
	cellSysmoduleLoadModule( CELL_SYSMODULE_AUDIO );

	cellFunc(MusicInitialize2,CELL_MUSIC2_PLAYER_MODE_NORMAL, 250, cb_musicutil, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand2, 0, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand2, 0, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand2, 2, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand2, 2, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand2, 3, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand2, 4, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand2, 1, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand2, 1, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand2, 1, NULL);
	loop_util();
	CellMusicSelectionContext ctxt = {};
	CellSearchContentId content_id = {};
	reset_obj(ctxt, -1);
	cellFunc(MusicGetSelectionContext2, &ctxt);
	cellFunc(MusicGetContentsId2, &content_id);
	cellFunc(MusicFinalize2);
	loop_util();

	sys_memory_container_t container = 0;
	ENSURE_OK(sys_memory_container_create(&container, CELL_MUSIC_PLAYBACK_MEMORY_CONTAINER_SIZE));
	cellFunc(MusicInitialize,CELL_MUSIC_PLAYER_MODE_NORMAL, container, 50, cb_musicutil, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand, 0, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand, 0, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand, 2, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand, 2, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand, 3, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand, 4, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand, 1, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand, 1, NULL);
	loop_util();
	cellFunc(MusicSetPlaybackCommand, 1, NULL);
	loop_util();
	CellMusicSelectionContext ctxt2 = {};
	CellSearchContentId content_id2 = {};
	reset_obj(ctxt2, -1);
	cellFunc(MusicGetSelectionContext, &ctxt2);
	cellFunc(MusicGetContentsId, &content_id2);
	//loop_util();
	cellFunc(MusicFinalize);
	loop_util();
	print_obj(ctxt2);
	loop_util();
	printf("finished");
	return 0;
}