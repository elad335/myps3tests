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

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x100000)

static sys_ppu_thread_t main_tid;
static u32 signal_exit = 0;

inline u32 _sys_ppu_thread_get_join_state(int *isjoinable)
{
	system_call_1(SYS_PPU_THREAD_GET_JOIN_STATE, (uint32_t) isjoinable);
	return_to_user_prog(u32);
}

void threadEntry(u64)
{
	u64 status;
	const u32 ret = sys_ppu_thread_join(main_tid, &status);
	printf("Joining error code = 0x%x.\n", ret);
	store_vol(signal_exit, 1);
	sys_ppu_thread_exit(0);
}

int main() {

	int state, ret;
	ret = _sys_ppu_thread_get_join_state(NULL);
	printf("sys_ppu_thread_get_join_state(NULL) returned:0x%x\n", ret);
	ret = _sys_ppu_thread_get_join_state(&state);
	printf("sys_ppu_thread_get_join_state returned: 0x%x, join_state=%d \n", ret, state);

	sys_ppu_thread_get_id(&main_tid);

	sys_ppu_thread_t m_tid;
	sys_ppu_thread_create(&m_tid,threadEntry,0,1002, 0x100000,1,"t");

	while (!load_vol(signal_exit))
	{
		sys_timer_usleep(300);
	}

	printf("sample finished.\n");
	return 0;
}