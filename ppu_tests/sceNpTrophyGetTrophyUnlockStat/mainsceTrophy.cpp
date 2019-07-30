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
#include <np.h>
#include <cell/cell_fs.h>
#include <memory>
#include <iostream>
#include <string>
#include <list>
#include <map>

#include "../ppu_header.h"

extern SceNpCommunicationId commId;
extern SceNpCommunicationSignature commSign; 

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

sys_memory_t mem_id;

sys_addr_t addr1;
sys_addr_t addr2;
sys_event_queue_t queue_id;

u8 commIdDat[128] = { 78, 80, 87, 82, 48, 48, 54, 52, 49, 0, 0, 0, 115, 99, 101, 78, 112, 77, 97, 110, 97, 103, 101, 114, 71, 101, 116, 67, 111, 110, 116, 101, 110, 116, 82, 97, 116, 105, 110, 103, 70, 108, 97, 103, 40, 37, 100, 44, 37, 100, 41, 32, 61, 61, 32, 37, 100, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 84, 114, 121, 67, 111, 110, 110, 101, 99, 116, 65, 115, 121, 110, 99, 0, 66, 67, 84, 114, 121, 67, 111, 110, 110, 101, 99, 116, 84, 97, 115, 107, 0, 0, 0, 0, 0, 0, 0, 0, 99, 101, 108, 108, 78, 101, 116, 67, 116, 108, 78, 101, 116, 83, 116, 97, 114, 116, 68, 105};
u8 commSignDat[128] = { 185, 221, 225, 59, 1, 0, 0, 0, 0, 0, 0, 0, 82, 104, 60, 186, 10, 230, 117, 248, 65, 40, 116, 14, 147, 138, 251, 77, 214, 254, 249, 16, 191, 105, 221, 248, 248, 76, 105, 171, 22, 245, 249, 169, 181, 58, 223, 51, 20, 128, 213, 202, 106, 54, 141, 75, 231, 59, 98, 97, 12, 143, 245, 255, 155, 19, 138, 31, 59, 112, 193, 92, 228, 136, 22, 131, 62, 152, 140, 39, 232, 106, 45, 136, 10, 100, 169, 247, 69, 118, 105, 63, 151, 18, 189, 115, 53, 70, 22, 210, 212, 128, 66, 165, 108, 108, 41, 8, 136, 102, 27, 141, 80, 119, 220, 144, 212, 167, 212, 251, 26, 155, 80, 34, 98, 46, 146, 86};

int complexCallback(SceNpTrophyContext context, SceNpTrophyStatus status, int completed, int total, void *arg)
{
	// Negative value aborts operation
	return 0;
};

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP_TROPHY );

	int ret = CELL_OK;

	sceNpTrophyInit(NULL, 0, SYS_MEMORY_CONTAINER_ID_INVALID, 0);

	SceNpTrophyHandle handle;
	ret |= sceNpTrophyCreateHandle(&handle);

	SceNpTrophyContext context;
	SceNpCommunicationId commId;
	SceNpCommunicationSignature commSign;
	memcpy(&commId, ptr_cast(commIdDat), 128);
	memcpy(&commSign, ptr_cast(commSignDat), 128);

	ret |= sceNpTrophyCreateContext(&context, &commId, &commSign, 0);





	sceNpTrophyTerm();
    printf("sample finished, ret=0x%x\n", ret);

    return 0;
}