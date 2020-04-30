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
#include <sysutil/sysutil_sysparam.h>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1, 0x100000)

int main() {

	CellAudioOutConfiguration cfg;
	for (u32 i = 0; i < 3; i++)
	{
		reset_obj(cfg, 0xff);
		cellFunc(AudioOutGetConfiguration, i, &cfg, NULL);
		print_obj(cfg);
	}

	u32 nums[3] = {0, 0, 0};
	nums[0] = cellFunc(AudioOutGetNumberOfDevice,0);
	nums[1] = cellFunc(AudioOutGetNumberOfDevice,1);
	cellFunc(AudioOutGetNumberOfDevice,2);

	CellAudioOutDeviceInfo info;
	CellAudioOutState state;

	for (u32 i = 0; i < 3; i++)
	{
		printf("device %d\n", i);

		for (u32 j = 0; j <= nums[i] + 1; j++)
		{
			reset_obj(info, 0xff);
			cellFunc(AudioOutGetDeviceInfo, i, j, &info);
			print_obj(info);
		}
	}


	for (u32 i = 0; i < 3; i++)
	{
		printf("device %d\n", i);

		for (u32 j = 0; j <= nums[i] + 1; j++)
		{
			reset_obj(state, 0xff);
			cellFunc(AudioOutGetState, i, j, &state);
			print_obj(state);
		}
	}

	return 0;
}