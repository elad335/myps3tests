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
#include <memory>
#include <iostream>
#include <string>
#include <list>
#include <map>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
// Allows for cache id to be 32 characters long (not including null term)
#define CELL_SDK_VERSION 1&0x36FFFF
SYS_PROCESS_PARAM(1000, 0x10000)

int main(int argc, const char * const argv[])
{
	const s32 sdk_ver = sys_process_get_sdk_version();

	printf("sdk version:0x%llx\n", sdk_ver);

	sys_ppu_thread_t thread_id = 0;
	ENSURE_OK(sys_ppu_thread_get_id(&thread_id));

	int priop = 0;
	ENSURE_OK(sys_ppu_thread_get_priority(thread_id, &priop));

	sys_ppu_thread_stack_t sp = {};
 	ENSURE_OK(sys_ppu_thread_get_stack_information(&sp));
 
	printf("Thread ID: 0x%x, PRIO: 0x%x, Stack size: 0x%x, Stack addr: 0x%x\n", thread_id, priop, sp.pst_size, sp.pst_addr);

	sys_memory_info_t mem_info[3] = {};
	ENSURE_OK(sys_memory_get_user_memory_size(mem_info));
	printf("Default memory container info:\n");
	print_obj(mem_info[0]);

	if (argc == 2)
	{
		sys_memory_container_t container = load_vol<u32>(*(u32*)argv[1]);
		container &= ~0xFFFF00;
		if (!sys_memory_container_get_size(&mem_info[1], container))
		{
			printf("Found memory container, ID=0x%x:\n", container);
			print_obj(mem_info[1]);
		}
	}

	sys_memory_container_t container = 0;
	sysCell(memory_container_create, &container, MB(1));
	printf("Created memory container which encapsulates 1MB of memory. (ID=0x%x)\n", container);

	if (!g_ec)
	{
		ENSURE_OK(sys_memory_get_user_memory_size(&mem_info[2]));
		printf("Default memory container info:\n");
		print_obj(mem_info[2]);
	}

	container |= 0xFFFF00; // Allow to be passed as C-string by hiding zeroes
	std::string path = argv[0];
	path.resize(path.size() - 5);
	path += "2.self"; // ppumain2.self
	printf("process finished -> exitspawn (arg1=\"%s\")\n", path.c_str());
	char* ptrs[2] = {(char*)&container, NULL};
	exitspawn(path.c_str(), ptrs, NULL, 0, 0, 1002, SYS_PROCESS_PRIMARY_STACK_SIZE_512K);

	printf("Unreachable!\n");
	return 0;
}