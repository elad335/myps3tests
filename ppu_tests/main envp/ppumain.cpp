#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>
#include <sys/prx.h>
#include <sys/spu_initialize.h> 

#include <sysutil/sysutil_msgdialog.h>
#include <sysutil/sysutil_syscache.h>
#include <cell/gem.h>
#include <cell/spurs.h> 
#include <cell/sysmodule.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <cell/camera.h> 
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <cell/pad.h>
#include <functional>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

void print_stacks()
{
	sys_page_attr_t attr;

	for (u32 i = 0xdu << 28, start_valid = 0; i <= 0xeu << 28; i+= 4096)
	{
		if (i == (0xeu << 28) || sys_memory_get_page_attribute(i, &attr))
		{
			if (start_valid)
			{
				printf("Stack: begin=0x%x, size=0x%x\n", start_valid, i - start_valid);
				print_bytes((void*)start_valid, i - start_valid);
			}

			start_valid = 0;
			continue;
		}

		if (!start_valid)
		{
			print_obj(attr);
			start_valid = i;
		}

		continue;
	}

}
int main(int argc, char *argv[], char *envp[])
{
	printf("r1=0x%x, argv=0x%x, envp=0x%x\n", GetGpr(1), (u32)argv, (u32)envp);

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_GEM );
	cellSysmoduleLoadModule( CELL_SYSMODULE_CAMERA );

	//cellPadInit(7);
	sys_timer_sleep(1);

	print_stacks();

	if (argc == 2 && strcmp(argv[1], "\\") == 0)
	{
		printf("sample finished.\n");
		return 0;
	}

	printf("process finished -> exitspawn (arg1=\"%s\")\n", argc ? argv[0] : "");

	char arg[2] = "\\";
	char* argv_sup[2] = {arg, NULL};
	char* envp_sup[2] = {arg, NULL};

	exitspawn(argv[0], argv_sup, envp_sup , 0, 0, 1002, SYS_PROCESS_PRIMARY_STACK_SIZE_512K);

	return 0;
}