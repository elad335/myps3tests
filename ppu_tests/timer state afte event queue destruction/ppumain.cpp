#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/timer.h> 
#include <sys/sys_time.h>
#include <functional>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack	: 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_event_queue_t qu = 0;
sys_event_queue_attribute_t ev_attr;


int ret = 0;
static volatile u32 _exit = 0;

void _rec(u64)
{
	while(true)
	{
		sys_event_t evt;
		u32 error = sys_event_queue_receive(qu, &evt, 8000);

		if (error == ESRCH) sys_timer_usleep(300);
		else if (error == CELL_OK) printf("Received an event\n");

		if (_exit)
		{
			break;
		}
	}

	sys_ppu_thread_exit(0);
}

int main() 
{
	sys_ppu_thread_t main_tid, waiter_tid[2];
	sys_ppu_thread_get_id(&main_tid);

	ENSURE_OK(sys_ppu_thread_create(&waiter_tid[0], &_rec, 0, 1001, 0x10000, 1, "T"));

	sys_event_queue_attribute_initialize(ev_attr);
	ENSURE_OK(sys_event_queue_create(&qu, &ev_attr, 0, 126));

	sys_event_port_t ep;
	ENSURE_OK(sys_event_port_create(&ep, 1, SYS_EVENT_PORT_NO_NAME));

	sys_timer_t timer;
	ENSURE_OK(sys_timer_create(&timer));
	u64 next = sys_time_get_system_time() + 2000;
	ENSURE_OK(sys_timer_connect_event_queue(timer, qu, 0, 0, 0));
	ENSURE_OK(sys_timer_start_periodic(timer, 5000));

	// Destro the event queue
	ENSURE_OK(sys_event_queue_destroy(qu, SYS_EVENT_QUEUE_DESTROY_FORCE));

	while (true)
	{
		sys_timer_usleep(1000);
		sys_timer_information_t t_info;
		sys_timer_get_information(timer, &t_info);

		// Wait for timer to stop
		if (t_info.timer_state == SYS_TIMER_STATE_STOP)
		{
			break;
		}
	}

	_exit = 1;
	ENSURE_VAL(sys_ppu_thread_join(waiter_tid[0], NULL), EFAULT);
	ENSURE_OK(sys_timer_disconnect_event_queue(timer));
	ENSURE_OK(sys_timer_destroy(timer));

	printf("sample finnished\n");
	return 0;
}