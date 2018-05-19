
// SysCalls
#define sys_rsx_context_attribute(a1, a2 , a3, a4, a5, a6) {system_call_6(SYS_RSX_CONTEXT_ATTRIBUTE, a1, a2 , a3, a4, a5, a6);}
#define sys_rsx_device_map(addr1, addr2, dev_id) {system_call_3(SYS_RSX_DEVICE_MAP, int_cast(addr1), int_cast(addr2), dev_id);} // dev_id = 0x8, addr2 is unused
#define sys_rsx_memory_allocate(mem_handle, mem_addr, size, flags, a5, a6, a7)  {system_call_7(SYS_RSX_MEMORY_ALLOCATE, int_cast(mem_handle), int_cast(mem_addr) ,size, flags, a5, a6, a7);} // size=0xf900000, flags=0x80000, a5=0x300000, a6=0xf, a7=0x8
#define sys_rsx_device_open() {system_call_0(SYS_RSX_DEVICE_OPEN);}