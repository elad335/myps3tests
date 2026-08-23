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
#include <memory>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000);

static sys_memory_t mem_id; 
static sys_addr_t addr1, addr2 = 0;

int main() 
{
	sysCell(hid_manager_map_logical_id_to_port_id, 1, 0, 0);
	u32 dev_id = -1;
	sysCell(hid_manager_open, 1, 0, &dev_id);

	u8 bufs[512];
	memset(bufs, 0, sizeof(bufs));

	sysCell(hid_manager_ioctl, dev_id, 0xb, bufs, 8);

	sys_event_queue_t equeue_id = 0;
	sys_event_queue_attribute_t attr;
	sys_event_queue_attribute_initialize(attr);
	sysCell(event_queue_create, &equeue_id, &attr, 0x8006020000000070ULL + 1, 127);

	u32 storages = 0;
	u32 devices = 0;
	sysCell(storage_get_device_config, &storages, &devices);

	printf("\nstorages: 0x%x, devices=0x%x", storages, devices);

	u32 ss_fd1 = 0;
	sysCell(storage_configure_medium_event, 0x0, equeue_id, &ss_fd1);

	u64* device_ids = new u64[devices + 1];
	const u64 sizeof_devices = sizeof(u64) * devices;

	memset(device_ids, 0xff, sizeof_devices);
	sysCell(storage_report_devices, 0x6, 0x0, 0x11, device_ids);
	printf("\ndevices:\n");
	print_bytes(device_ids, sizeof_devices);

	StorageDeviceInfo device_info;
	reset_obj(device_info);
	sysCell(storage_get_device_info, 0x101000000000006ULL, &device_info);

	u32 ss_fd = 0;
	sysCell(storage_open, 0x101000000000006ULL, 0x0, &ss_fd, 0x0);

	print_bytes(bufs, 8);

	printf("sample finished.\n");

	return 0;
}