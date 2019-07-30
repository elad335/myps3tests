#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>

#include <sysutil/sysutil_syscache.h>
#include <cell/sysmodule.h>
#include <cell/pad.h>
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

inline void trap_syscall() { asm volatile ("twi 0x10, 3, 0"); }; 

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;

sys_addr_t addr1;
sys_addr_t addr2;
sys_event_queue_t queue_id;

void printHwords(u16* data, size_t size)
{
	for (u32 i = 0; i < size; i++)
	{
		printf("[%04x] %04X\n", i, data[i]);
	}	
}

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_IO );

	register int ret asm ("3");

	if (cellPadInit(1) != CELL_OK)
	{
		printf("Fatal init error returned\n");
		return 0;
	}

	printf("Getting connected controlllers..\n");

	u32 port = -1;

	do
	{
		CellPadInfo2 info;
		cellPadGetInfo2(&info);

		for (u32 i = 0; i < CELL_PAD_MAX_PORT_NUM; i++)
		{
			if (info.port_status[i] & CELL_PAD_STATUS_CONNECTED)
			{
				port = i;
			}
		}

		sys_timer_usleep(1000);

	} while (port > 7);

	sys_timer_usleep(1000);

	cellPadSetPortSetting(port,0);

	printf("Press on a face buttom and release..\n");

	sys_timer_sleep(1);

	//cellPadClearBuf(port);
	CellPadData data;

	do
	{
		memset(&data, 0xff, sizeof(data));
		cellPadGetData(port, &data);
	} while(data.len == 0);

	printf("data lengh: %x\n", data.len);

	printf("data1 = \n");
	printHwords(ptr_caste(data.button, u16), CELL_PAD_MAX_CODES);
	printf("\n");

	cellPadSetPortSetting(port, CELL_PAD_SETTING_PRESS_ON);

	printf("Rest a little now..\n");

	sys_timer_sleep(1);

	//cellPadClearBuf(port);


	do
	{
		memset(&data, 0xff, sizeof(data));
		cellPadGetData(port, &data);
	} while(data.len != 0);

	printf("data lengh: %x\n", data.len);

	printf("null data = \n");
	printHwords(ptr_caste(data.button, u16), CELL_PAD_MAX_CODES);
	printf("\n");

	printf("Press any face buttom..\n");

	do
	{
		memset(&data, 0xff, sizeof(data));
		cellPadGetData(port, &data);
	} while(data.len == 0);

	printf("data lengh: %x\n", data.len);

	printf("data2 = \n");
	printHwords(ptr_caste(data.button, u16), CELL_PAD_MAX_CODES);
	printf("\n");

	printf("Now chill..\n");

	sys_timer_usleep(1);

	cellPadSetPortSetting(port, CELL_PAD_SETTING_SENSOR_ON);
	//cellPadClearBuf(port);
	cellPadGetData(port, &data);

	do
	{
		memset(&data, 0xff, sizeof(data));
		cellPadGetData(port, &data);
	} while(data.len == 0);

	printf("data lengh: %x\n", data.len);

	printf("data3 = \n");
	printHwords(ptr_caste(data.button, u16), CELL_PAD_MAX_CODES);
	printf("\n");

	cellPadSetPortSetting(port, CELL_PAD_SETTING_SENSOR_ON | CELL_PAD_SETTING_PRESS_ON);
	//cellPadClearBuf(port);
	cellPadGetData(port, &data);

	do
	{
		memset(&data, 0xff, sizeof(data));
		cellPadGetData(port, &data);
	} while(data.len == 0);

	printf("data lengh: %x\n", data.len);

	printf("data4 = \n");
	printHwords(ptr_caste(data.button, u16), CELL_PAD_MAX_CODES);
	printf("\n sample finished.\n");

    return 0;
}