#include <stdint.h>
#include <cell/sysmodule.h>
#include <cell/atomic.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>
#include <cstring>
#include <string>

#define int_cast(p) reinterpret_cast<uintptr_t>(p)
#define ptr_cast(x) reinterpret_cast<void*>(x)
#define ptr_caste(x, type) reinterpret_cast<type*>(ptr_cast(x))
#define ref_cast(x, type) *reinterpret_cast<type*>(ptr_cast(x))

typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef intptr_t sptr;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef float f32;
typedef double f64; 

#define ALIGN(x) __attribute__((aligned(x)))
#define STR_CASE(...) case __VA_ARGS__: return #__VA_ARGS__

static std::string format_cell_error(u32 error)
{
	switch (error)
	{
	case CELL_OK: return "ok";
	STR_CASE(EAGAIN);
	STR_CASE(EINVAL);
	STR_CASE(ENOSYS);
	STR_CASE(ENOMEM);
	STR_CASE(ESRCH);
	STR_CASE(ENOENT);
	STR_CASE(ENOEXEC);
	STR_CASE(EDEADLK);
	STR_CASE(EPERM);
	STR_CASE(EBUSY);
	STR_CASE(ETIMEDOUT);
	STR_CASE(EABORT);
	STR_CASE(EFAULT);
	//STR_CASE(ENOCHILD);
	STR_CASE(ESTAT);
	STR_CASE(EALIGN);
	STR_CASE(EKRESOURCE);
	STR_CASE(EISDIR);
	STR_CASE(ECANCELED);
	STR_CASE(EEXIST);
	STR_CASE(EISCONN);
	STR_CASE(ENOTCONN);
	STR_CASE(EAUTHFAIL);
	STR_CASE(ENOTMSELF);
	STR_CASE(ESYSVER);
	STR_CASE(EAUTHFATAL);
	STR_CASE(EDOM);
	STR_CASE(ERANGE);
	STR_CASE(EILSEQ);
	STR_CASE(EFPOS);
	STR_CASE(EINTR);
	STR_CASE(EFBIG);
	STR_CASE(EMLINK);
	STR_CASE(ENFILE);
	STR_CASE(ENOSPC);
	STR_CASE(ENOTTY);
	STR_CASE(EPIPE);
	STR_CASE(EROFS);
	STR_CASE(ESPIPE);
	STR_CASE(E2BIG);
	STR_CASE(EACCES);
	STR_CASE(EBADF);
	STR_CASE(EIO);
	STR_CASE(EMFILE);
	STR_CASE(ENODEV);
	STR_CASE(ENOTDIR);
	STR_CASE(ENXIO);
	STR_CASE(EXDEV);
	STR_CASE(EBADMSG);
	STR_CASE(EINPROGRESS);
	STR_CASE(EMSGSIZE);
	STR_CASE(ENAMETOOLONG);
	STR_CASE(ENOLCK);
	STR_CASE(ENOTEMPTY);
	STR_CASE(ENOTSUP);
	STR_CASE(EFSSPECIFIC);
	STR_CASE(EOVERFLOW);
	STR_CASE(ENOTMOUNTED);
	STR_CASE(ENOTSDATA);
	STR_CASE(ESDKVER);
	STR_CASE(ENOLICDISC);
	STR_CASE(ENOLICENT);
	default: break;
	}
	std::string buf("\0\0\0\0\0\0\0\0\0\0", 10);
	sprintf(&buf[0], "0x%x", (u32)error);
	return buf;
}

#ifndef ENSURE_OK

s32 ensure_ok_ret_save_ = 0; // Unique naming
#define ENSURE_OK(x) if ((ensure_ok_ret_save_ = s32(x)) != CELL_OK) { printf("\"%s\" Failed at line %d! (error=%s)", #x, __LINE__, format_cell_error(ensure_ok_ret_save_).c_str()); exit(ensure_ok_ret_save_); }

#endif

// Lazy memory barrier (missing basic memory optimizations)
#ifndef fsync 
#define fsync() __asm__ volatile ("sync" : : : "memory"); __asm__ volatile ("eieio")

template <typename T>
volatile T& as_volatile(T& obj)
{
	fsync();
	return const_cast<volatile T&>(obj);
}

template <typename T>
const volatile T& as_volatile(const T& obj)
{
	fsync();
	return const_cast<const volatile T&>(obj);
}

template <typename T, T value>
static const volatile T& as_volatile_v()
{
	static const volatile T v = value;
	return v;
}

#endif

#ifndef cellFunc

// TODO
s64 g_ec = 0;
#define cellFunc(name, ...) \
(printf("cell" #name "(error %s)\n", format_cell_error(s32(g_ec = cell##name(__VA_ARGS__))).c_str()), g_ec)
#define sceFunc(name, ...) \
(printf("sce" #name "(error %s)\n", format_cell_error(s32(g_ec = sce##name(__VA_ARGS__))).c_str()), g_ec)
#define sysCell(name, ...) \
(printf("sys_" #name "(error %s)\n", format_cell_error(s32(g_ec = sys_##name(__VA_ARGS__))).c_str()), g_ec)
#endif

template <typename To, typename From>
static inline To bit_cast(const From& from)
{
	To to;
	std::memcpy(&to, &from, sizeof(From));
	return to;
}

#define thread_exit(x) sys_ppu_thread_exit(x)
#define thread_eoi sys_interrupt_thread_eoi
#define thread_create sys_ppu_thread_create
#define thread_join sys_ppu_thread_join

void printBytes(const volatile void* data, size_t size)
{
	fsync();

	const volatile u8* bytes = static_cast<const volatile u8*>(data);

	for (u32 i = 0; i < size;)
	{
		if (size > 0xfff)
		{
			printf("[%04x] |", i);
		}
		else
		{
			printf("[%03x] |", i);
		}
		

		switch (size - i)
		{
		default: printf(" %02X", bytes[i++]);
		case 7: printf(" %02X", bytes[i++]);
		case 6: printf(" %02X", bytes[i++]);
		case 5: printf(" %02X |", bytes[i++]);
		case 4: printf(" %02X", bytes[i++]);
		case 3: printf(" %02X", bytes[i++]);
		case 2: printf(" %02X", bytes[i++]);
		case 1: printf(" %02X |", bytes[i++]);
		}

		printf("\n");
	}	
}

template <typename T>
void printType(const volatile T& obj)
{
	printBytes(&obj, sizeof(obj));
}

#define GetGpr(reg) \
({ \
	register uint64_t p1 __asm__ (#reg); \
	p1; \
})

#define SetGpr(reg, value) \
({ \
	register uint64_t p1 __asm__ (#reg) = value; \
	value; \
})

// Hack: use volatile load to ensure UB won't cause bugged behaviour
static u32 lv2_lwcond_wait(sys_lwcond_t* lwc, sys_lwmutex_t* mtx, u64 timeout)
{
	system_call_3(0x071, as_volatile(*ptr_caste(int_cast(lwc) + 4, u32)), as_volatile(*ptr_caste(int_cast(mtx) + 16, u32)), timeout);
	return_to_user_prog(u32);
}

static u32 lv2_lwcond_signal(sys_lwcond_t* lwc, sys_lwmutex_t* mtx, u32 ppu_id, u32 mode)
{
	system_call_4(0x073, as_volatile(*ptr_caste(int_cast(lwc) + 4, u32)), as_volatile(*ptr_caste(int_cast(mtx) + 16, u32)), ppu_id, mode);
	return_to_user_prog(u32);
}

static u32 lv2_lwcond_signal_all(sys_lwcond_t* lwc, sys_lwmutex_t* mtx, u32 mode)
{
	system_call_3(0x074, as_volatile(*ptr_caste(int_cast(lwc) + 4, u32)), as_volatile(*ptr_caste(int_cast(mtx) + 16, u32)), mode);
	return_to_user_prog(u32);
}

static u32 lv2_lwmutex_lock(sys_lwmutex_t* mtx, u64 timeout)
{
	system_call_2(0x061, *ptr_caste(int_cast(mtx) + 16, u32), timeout);
	return_to_user_prog(u32);
}

static u32 lv2_lwmutex_trylock(sys_lwmutex_t* mtx)
{
	system_call_1(0x063, as_volatile(*ptr_caste(int_cast(mtx) + 16, u32)));
	return_to_user_prog(u32);
}

static u32 lv2_lwmutex_unlock(sys_lwmutex_t* mtx)
{
	system_call_1(0x062, as_volatile(*ptr_caste(int_cast(mtx) + 16, u32)));
	return_to_user_prog(u32);
}

static u32 sys_overlay_load_module(u32* ovlmid, char* path2, u64 flags, u32* entry)
{
	system_call_4(0x1C2, int_cast(ovlmid), int_cast(path2), flags, int_cast(entry));
	return_to_user_prog(u32);
}

static u32 sys_overlay_unload_module(u32 ovlmid)
{
	system_call_1(0x1C3, ovlmid);
	return_to_user_prog(u32);
}

static u32 sys_process_get_sdk_version_impl(u32 pid, u32* out)
{
	system_call_2(SYS_PROCESS_GET_SDK_VERSION, pid, int_cast(out));
	return_to_user_prog(s32);
}

static s64 sys_process_get_sdk_version(u32 pid = sys_process_getpid())
{
	u32 sdk_ver = 0;
	const u32 ret = sys_process_get_sdk_version_impl(pid, &sdk_ver);
	return s64(ret ? ((0ull - 1) << 32) | ret : sdk_ver);
}

static u32 sys_fs_test(u32 arg1, u32 arg2, u32* fd, u32 u32_size, char* out, u32 out_size)
{
	system_call_6(0x320, arg1, arg2, int_cast(fd), u32_size, int_cast(out), out_size);
	return_to_user_prog(u32);
}

static u64 sys_hid_manager_is_process_permission_root(u32 pid = sys_process_getpid())
{
	system_call_1(512,pid);
	return_to_user_prog(u64);
}

struct CellFsMountInfo
{
	char mount_path[0x20]; // 0x0
	char filesystem[0x20]; // 0x20
	char dev_name[0x40];   // 0x40
	u32 unk1;        // 0x80
	u32 unk2;        // 0x84
	u32 unk3;        // 0x88
	u32 unk4;        // 0x8C
	u32 unk5;        // 0x90
};

struct mmapper_unk_entry_struct0
{
	u32 a;    // 0x0
	u32 b;    // 0x4
	u32 c;    // 0x8
	u32 d;    // 0xc
	u64 type; // 0x10
};

static u32 sys_fs_get_mount_info_size(u64* len)
{
	system_call_1(0x349, int_cast(len));
	return_to_user_prog(u32);
}

static u32 sys_fs_get_mount_info(CellFsMountInfo* info, u64 len, u64* out_len)
{
	system_call_3(0x34A, int_cast(info), len, int_cast(out_len));
	return_to_user_prog(u32);
}

#define sys_fs_ftruncate2 sys_fs_truncate2
static u32 sys_fs_truncate2(s32 fd, u32 size)
{
	system_call_2(0x34F, fd + 0u, size);
	return_to_user_prog(u32);	
}

static u32 sys_mmapper_allocate_shared_memory(u64 ipc_key, u32 size, u64 flags, u32* mem_id)
{
	system_call_4(332, ipc_key, size, flags, int_cast(mem_id));
	return_to_user_prog(u32);
}

static u32 sys_mmapper_allocate_shared_memory_from_container(u64 ipc_key, u32 size, u32 cid, u64 flags, u32* mem_id)
{
	system_call_5(362, ipc_key, size, cid, flags, int_cast(mem_id));
	return_to_user_prog(u32);
}

static u32 sys_mmapper_allocate_shared_memory_ext(u64 ipc_key, u32 size, u32 flags, mmapper_unk_entry_struct0* entries, s32 count, u32* mem_id)
{
	system_call_6(339, ipc_key, size, flags, int_cast(entries), count, int_cast(mem_id));
	return_to_user_prog(u32);
}

static u32 sys_mmapper_allocate_shared_memory_from_container_ext(u64 ipc_key, u32 size, u64 flags, u32 mc_id, mmapper_unk_entry_struct0* entries, s32 entry_count, u32* mem_id)
{
	system_call_7(328, ipc_key, size, flags, mc_id, int_cast(entries), entry_count, int_cast(mem_id));
	return_to_user_prog(u32);
}

static u32 sys_ss_random_number_generator(u64 pkg_id, void* buf, u64 size)
{
	system_call_3(0x361, pkg_id, int_cast(buf), size);
	return_to_user_prog(u32);
}

static u32 cellFsGetPath_s(u32 fd, char* out_path)
{
	if (!out_path)
	{
		return EFAULT;
	}

	char p[0x420] = {};
	const u32 res = sys_fs_test(6, 0, &fd, sizeof(u32), p, 0x420);

	if (res)
	{
		return res;
	}

	std::memcpy(out_path, p, std::strlen(p) + 1);
	return CELL_OK;
}