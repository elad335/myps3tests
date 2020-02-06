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
#include <sysutil/sysutil_gamecontent.h> 

#include <np.h>
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
//#define CELL_SDK_VERSION 0x00210000
SYS_PROCESS_PARAM(1000, 0x100000)


int complexCallback(SceNpTrophyContext context, SceNpTrophyStatus status, int completed, int total, void *arg)
{
	// Negative value aborts operation
	return 0;
};

const char commids[] = "NPWR03045"; // 45
const char comms[] = "b9dde13b0100000000000000d4dd7fc7603f2b2bc6a68599d6d94bea4846a18612a40c914a16318db05331362ec421a3e465976b48a5ae43c9aaf38260b153a421bfa3ea439d12527af20b43af45c4a9029a57a8c7e318cbaf71b786d4efc6ecc24d9c50d02961b223ae4bfe299b3f25158bba21d864fbf983ef9a267084ec95c3b6b3d954941d4129b6e7f422c6b56b6fbb1c674044651af9118936c21b23a7";

u8 fromHexChar(char c_)
{
	u8 c = bit_cast<u8>(c_);

	if (c >= '0' && c <= '9')
	{
		return (c -= '0');
	}

	if (c >= 'a' && c <= 'f')
	{
		return (c -= 'a') + 10;
	}

	if (c >= 'A' && c <= 'F')
	{
		return (c -= 'A') + 10;
	}

	ENSURE_OK(true | (c << 8));
	return 0xff;
}

void fromHexStr(u8* data, const char* str, size_t size)
{
	for (u32 i = 0; i < size; i += 2)
	{
		u8 build = 0;

		build |= fromHexChar( str[i] ) << 4;
		build |= fromHexChar( str[i+1] );
		data[i/2] = build;
	}
}

int main() {

	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_GAME );
	cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_NP_TROPHY );

	sceNpTrophyInit(NULL, 0, SYS_MEMORY_CONTAINER_ID_INVALID, 0);

	SceNpTrophyHandle handle;

	SceNpTrophyContext context;
	SceNpCommunicationId commId;
	SceNpCommunicationSignature commSign;
	memset(commSign.data, 0, sizeof (commSign.data));
	fromHexStr(commSign.data, comms, strlen(comms));
	memset(&commId, 0, sizeof(commId));
	strcpy(commId.data, commids);
	commId.term = '0';

	ENSURE_OK(sceNpTrophyCreateContext(&context, &commId, &commSign, 0));

	ENSURE_OK(sceNpTrophyCreateHandle(&handle));

	u32 count = 0;

	SceNpTrophyFlagArray flags;
	std::memset(flags.flag_bits, 0xff, 16);
	sceFunc(NpTrophyGetTrophyUnlockState, context, handle, &flags, &count);

	u64 req = 0;
	sceFunc(NpTrophyGetRequiredDiskSpace, context, handle, &req, 0);
	printf("sample finished. (req=0x%llx, flags=0x%x%x%x%x)\n", req, flags.flag_bits[0], flags.flag_bits[1], flags.flag_bits[2], flags.flag_bits[3]);
	return 0;
}