#ifndef __HEADER_GUARD_ELADPPU_HEADER__
#define __HEADER_GUARD_ELADPPU_HEADER__
#include <stdint.h>
#include <cell/sysmodule.h>
#include <cell/atomic.h>
#include <cell/gcm.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>
#include <cell/cell_fs.h> 
#include <ppu_intrinsics.h>
#include <ppu_asm_intrinsics.h>
#include <sysutil/sysutil_gamecontent.h> 
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

// Some headers in define it this way
#undef vec_u8;
#undef vec_s8;
#undef vec_u16;
#undef vec_s16;
#undef vec_u32;
#undef vec_s32;
//#undef vec_f32;

typedef vector unsigned char vec_u8;
typedef vector signed char vec_s8;
typedef vector unsigned short vec_u16;
typedef vector signed short vec_s16;
typedef vector unsigned int vec_u32;
typedef vector signed int vec_s32;
typedef vector float vec_f32;

#define decltype __typeof__
#define thread_local __thread

#define ALIGN(x) __attribute__((aligned(x)))
#define STR_CASE(...) case __VA_ARGS__: return #__VA_ARGS__

#define MB(x) ((x) * (1ull<<20))

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

// Basic asserts, uses c-style casts in some places handle pointers as well
#define ENSURE_OK(x) ({ if (s32 ensure_ok_ret_save_ = static_cast<s64>(x)) { printf("\"%s\" Failed at line %d! (error=%s)", #x, __LINE__, format_cell_error(ensure_ok_ret_save_).c_str()); exit(ensure_ok_ret_save_); } 0; })
#define ENSURE_VAL(x, val) ({ s64 ensure_ok_ret_save_ = s64(x); if (ensure_ok_ret_save_ != s64(val)) { printf("\"%s\" Failed at line %d! (error=%s)", #x, __LINE__, format_cell_error(ensure_ok_ret_save_).c_str()); exit(ensure_ok_ret_save_); } 0; })
#define ENSURE_NVAL(x, val) ({ s64 ensure_ok_ret_save_ = s64(x); if (ensure_ok_ret_save_ == s64(val)) { printf("\"%s\" Failed at line %d! (error=%s)", #x, __LINE__, format_cell_error(ensure_ok_ret_save_).c_str()); exit(ensure_ok_ret_save_); } 0; })

#endif

// Lazy memory barrier (missing basic memory optimizations)
#ifndef fsync 
#define fsync() ({ __sync(); __eieio(); })

#define CELL_FS_FLAG(x) O_##x = CELL_FS_O_##x
enum
{
	CELL_FS_FLAG(RDONLY),
	CELL_FS_FLAG(RDWR),
	CELL_FS_FLAG(WRONLY),
	CELL_FS_FLAG(APPEND),
	CELL_FS_FLAG(CREAT),
	CELL_FS_FLAG(TRUNC),
	CELL_FS_FLAG(EXCL),
	CELL_FS_FLAG(MSELF),
};

// Trivial "atomic" type read/write for trivial types
// atomic, prevents stores/loads reordering, works on all memory area types (cache inhibited etc)
template <typename T>
T load_vol(const volatile T& obj)
{
	fsync();
	const T ret = obj;
	__lwsync();
	return ret;
}

template <typename T>
T store_vol(volatile T& obj, s64 value)
{
	fsync();
	obj = static_cast<T>(value);
	fsync();
	return value;
}

// Workaround for old compiler limitations (no universal reference)
template <typename T>
T store_vol_f(volatile T& obj, f64 value)
{
	fsync();
	obj = static_cast<T>(value);
	fsync();
	return value;
}

template <typename T>
T* store_vol(T* volatile& obj, T* value)
{
	fsync();
	obj = value;
	fsync();
	return value;
}

template <typename T>
volatile T* store_vol(volatile T* volatile& obj, volatile T* value)
{
	fsync();
	obj = value;
	fsync();
	return value;
}

template <typename T>
const T* store_vol(const T* volatile& obj, const T* value)
{
	fsync();
	obj = value;
	fsync();
	return value;
}

template <typename T>
const volatile T* store_vol(const volatile T* volatile& obj, const volatile T* value)
{
	fsync();
	obj = value;
	fsync();
	return value;
}

// Volatile constant, prevents compiler optimizations
template <typename T, T value>
static const volatile T& constant_vol()
{
	static const volatile T v = value;
	return v;
}

#endif

#ifndef cellFunc

// Logging utilities macros

static thread_local s64 g_ec = 0; // Global error code
#define cellFunc(name, ...) \
(printf("cell" #name "(error %s, line=%u)\n", format_cell_error(s32(g_ec = cell##name(__VA_ARGS__))).c_str(), __LINE__), g_ec)
#define sceFunc(name, ...) \
(printf("sce" #name "(error %s, line=%u)\n", format_cell_error(s32(g_ec = sce##name(__VA_ARGS__))).c_str(), __LINE__), g_ec)
#define sysCell(name, ...) \
(printf("sys_" #name "(error %s, line=%u)\n", format_cell_error(s32(g_ec = sys_##name(__VA_ARGS__))).c_str(), __LINE__), g_ec)
#define justFunc(name, ...) \
(printf(#name "(error %s, line=%u)\n", format_cell_error(s32(g_ec = name(__VA_ARGS__))).c_str(), __LINE__), g_ec)
#define print1Func(name, ...) \
(printf(#name "(%s, line=%u)\n", format_cell_error(s32(g_ec = name(__VA_ARGS__))).c_str(), __LINE__), g_ec)
#endif

template <typename To, typename From>
static inline To bit_cast(const From& from)
{
	To to;
	std::memcpy(&to, &from, sizeof(From));
	return to;
}

namespace std
{
	template <typename T>
	u32 size(const T& array)
	{
		return sizeof(array) / sizeof(array[0]);
	}

	template <typename T>
	u32 size(const volatile T& array)
	{
		return sizeof(array) / sizeof(array[0]);
	}
}

#define thread_exit(x) sys_ppu_thread_exit(x)
#define thread_eoi() sys_interrupt_thread_eoi()
#define thread_create sys_ppu_thread_create
#define thread_join sys_ppu_thread_join

void print_bytes(const volatile void* data, size_t size)
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
void print_obj(const volatile T& obj, const char* name = NULL)
{
	if (name)
	{
		printf(name);
		printf(": ");
	}

	print_bytes(&obj, sizeof(obj));
}

#define PRINT_OBJ(x) print_obj(x, #x)

template <typename T>
void reset_obj(T& obj, int ch = 0)
{
	std::memset(&obj, ch, sizeof(obj));
}

template <typename T>
void reset_obj(volatile T& obj, int ch = 0)
{
	// TODO
	for (size_t i = 0; i < sizeof(obj); i++)
		reinterpret_cast<volatile char*>(&obj)[i] = 0;
}

template <typename T>
u64 xor_obj(T& dst, const T& src, const T& src2)
{
	u64 changes = 0;

	// Show bitwise difference
	for (size_t i = 0; i < sizeof(T); i++)
	{
		const u8 res = reinterpret_cast<const volatile char*>(&src)[i] ^ reinterpret_cast<const volatile char*>(&src2)[i];
		changes += res != 0;
		reinterpret_cast<volatile char*>(&dst)[i] = res;
	}

	return changes;
}

template <typename T>
u64 xor_obj(T& dst, const T& applied)
{
	return xor_obj(dst, dst, applied);
}

#define GetGpr(reg) \
({ \
	register uint64_t p_##reg_no_variable_name_read __asm__ (#reg); \
	p_##reg_no_variable_name_read; \
})

#define SetGpr(reg, value) \
({ \
	register uint64_t p_##reg_no_variable_name_write __asm__ (#reg) = value; \
	value; \
})

static s32 open_file_(s32 line, const char *path, int flags, const u64& arg = -1, u64 size_arg = 0)
{
	printf("line=%d, path=%s, flags=%u: ", line, path, flags);

	if (arg != -1 && size_arg == 0)
	{
		// u64 argument, only known to be used
		size_arg = 8;
	}

	// For other arg sizes: *(u64*)(arg_t*)&arg

	int fd = -1;
	const s32 err = cellFunc(FsOpen, path, flags, &fd, (void*)&arg, size_arg);

	if (err < 0)
	{
		// return error code as is
		return err;
	}

	// Sync just in case
	cellFsFsync(fd);

	return fd;
}

#define open_file(...) open_file_(__LINE__, __VA_ARGS__)
#define open_sdata(...) open_file(__VA_ARGS__, 0x18000000010ull)
#define open_edata(...) open_file(__VA_ARGS__, 0x2ull)

static s64 write_file_(s32 line, s32 fd, const void* buf, u64 size, bool is_verbose = true)
{
	if (is_verbose) printf("line=%d, fd=%d, size=%u: ", line, fd, size);

	u64 written = 0;
	const s32 err = (is_verbose ? cellFunc(FsWrite, fd, buf, size, &written) : cellFsWrite(fd, buf, size, &written));

	if (err < 0)
	{
		// Sign-extend error code
		return err;
	}

	cellFsFsync(fd);
	return written;
}

#define write_file(...) write_file_(__LINE__, __VA_ARGS__)
#define write_str_file(fd, str) write_file(fd, str.data(), str.size())

static s64 write_off_file_(s32 line, s32 fd, const void* buf, u64 size, u64 offset)
{
	printf("line=%d, fd=%d, size=%u, offset=0x%x: ", line, fd, size, offset);

	u64 written = 0;
	const s32 err = cellFunc(FsWriteWithOffset, fd, offset, buf, size, &written);

	if (err < 0)
	{
		// Sign-extend error code
		return err;
	}

	cellFsFsync(fd);
	return written;
}

#define write_off_file(...) write_off_file_(__LINE__, __VA_ARGS__)
#define write_str_off_file(line, fd, str, offset) write_off_file(line, fd, str.data(), str.size(), offset)

static CellFsStat stat_file_(s32 line, s32 fd, bool print_it = false)
{
	printf("line=%d, fd=%d: ", line, fd);

	CellFsStat stat;

	const s32 err = cellFunc(FsFstat, fd, &stat);

	if (err < 0)
	{
		reset_obj(stat, 0xFF);

		// Write error code to size
		// It also writes error code to g_ec anyways
		stat.st_size = err;
	}

	if (print_it)
	{
		print_obj(stat);
	}

	return stat;
}

static CellFsStat stat_file_(s32 line, const char* path, bool print_it = false)
{
	printf("line=%d, path=%s: ", line, path);

	CellFsStat stat;

	const s32 err = cellFunc(FsStat, path, &stat);

	if (err < 0)
	{
		reset_obj(stat, 0xFF);

		// Write error code to size
		// It also writes error code to g_ec anyways
		stat.st_size = err;
	}

	if (print_it)
	{
		print_obj(stat);
	}

	return stat;
}

#define stat_file(...) stat_file_(__LINE__, __VA_ARGS__)

static s64 read_file_(s32 line, s32 fd, void* buf, u64 size)
{
	printf("line=%d, fd=%d, size=0x%x: ", line, fd, size);

	u64 read = 0;
	const s32 err = cellFunc(FsRead, fd, buf, size, &read);

	if (err < 0)
	{
		// Sign-extend error code
		return err;
	}

	return read;
}

#define read_file(...) read_file_(__LINE__, __VA_ARGS__)
#define read_str_file(fd, str) read_file(fd, (void*)str.data(), str.size())

static s64 read_off_file_(s32 line, s32 fd, void* buf, u64 size, u64 offset)
{
	printf("line=%d, fd=%d, size=%u, offset=0x%x: ", line, size, offset);

	u64 read = 0;
	const s32 err = cellFunc(FsReadWithOffset, fd, offset, buf, size, &read);

	if (err < 0)
	{
		// Sign-extend error code
		return err;
	}

	return read;
}

#define read_off_file(...) read_off_file_(__LINE__, __VA_ARGS__)
#define read_str_off_file(fd, str, offset) read_off_file(fd, (void*)str.data(), str.size(), offset)

static s64 copy_file_(s32 line, const char* og_path, const char* dst_path)
{
	printf("line=%d, path=%s, dst=%s: ", line, og_path, dst_path);

	const s32 fd0 = open_file_(line, og_path, 0);

	if (fd0 < 0)
	{
		return g_ec;
	}

	const s32 fd1 = open_file_(line, dst_path, O_CREAT | O_RDWR | O_TRUNC);

	if (fd1 < 0)
	{
		cellFsClose(fd0);
		return g_ec;
	}

	char* ptr = new char[0x100000];

	for (u64 i = 0;; i += 0x100000)
	{
		const s64 read_size = read_file_(line, fd0, ptr, 0x100000);

		if (!read_size)
		{
			break;
		}

		write_file_(line, fd1, ptr, read_size);
	}

	cellFsFsync(fd1);
	cellFsClose(fd1);
	cellFsClose(fd0);
	delete ptr;
	return 0;
}

#define copy_file(...) copy_file_(__LINE__, __VA_ARGS__)

const std::string& create_hdd0_temp_data()
{
	static std::string usr;

	if (!usr.empty())
	{
		return usr;
	}

	cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_GAME);
	cellGameContentPermit(NULL, NULL);
	cellGameDeleteGameData("TEMPGD");

	ENSURE_VAL(cellGameDataCheck(CELL_GAME_GAMETYPE_GAMEDATA, "TEMPGD", NULL), CELL_GAME_RET_NONE);
	CellGameSetInitParams initp = {};
	memcpy(initp.titleId, "TEMPGD", 6);

	usr.resize(CELL_GAME_PATH_MAX);

	ENSURE_OK(cellGameCreateGameData(&initp, NULL, (char*)usr.data()));
	usr.resize(usr.find_first_of('\0'));
	usr += '/';
	return usr;
}
	
static u32 lv2_lwcond_wait(sys_lwcond_t* lwc, sys_lwmutex_t* mtx, u64 timeout)
{
	system_call_3(0x071, load_vol(lwc->lwcond_queue), load_vol(mtx->sleep_queue), timeout);
	return_to_user_prog(u32);
}

static u32 lv2_lwcond_signal(sys_lwcond_t* lwc, sys_lwmutex_t* mtx, u32 ppu_id, u32 mode)
{
	system_call_4(0x073, load_vol(lwc->lwcond_queue), load_vol(mtx->sleep_queue), ppu_id, mode);
	return_to_user_prog(u32);
}

static u32 lv2_lwcond_signal_all(sys_lwcond_t* lwc, sys_lwmutex_t* mtx, u32 mode)
{
	system_call_3(0x074, load_vol(lwc->lwcond_queue), load_vol(mtx->sleep_queue), mode);
	return_to_user_prog(u32);
}

static u32 lv2_lwmutex_lock(sys_lwmutex_t* mtx, u64 timeout)
{
	system_call_2(0x061, load_vol(mtx->sleep_queue), timeout);
	return_to_user_prog(u32);
}

static u32 lv2_lwmutex_trylock(sys_lwmutex_t* mtx)
{
	system_call_1(0x063, load_vol(mtx->sleep_queue));
	return_to_user_prog(u32);
}

static u32 lv2_lwmutex_unlock(sys_lwmutex_t* mtx)
{
	system_call_1(0x062, load_vol(mtx->sleep_queue));
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

static u32 sys_net_infoctl(u32 cmd, void* buf)
{
	system_call_2(722, cmd, int_cast(buf));
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

u32 sys_rsx_memory_allocate(u32* mem_handle, u64* mem_addr)
{
	// Note: memory_size set to  0xf200000 as passed from sdk version 1.9 <= x < 2.2
	system_call_7(SYS_RSX_MEMORY_ALLOCATE, int_cast(mem_handle), int_cast(mem_addr), 0xf200000, 0x80000, 0x300000, 0xf, 0x8);
	return_to_user_prog(u32);
}

u32 sys_rsx_context_allocate(u32* context_id, u64* lpar_dma_control, u64* lpar_driver_info, u64* lpar_reports, u32 mem_handle)
{
	system_call_6(SYS_RSX_CONTEXT_ALLOCATE, int_cast(context_id), int_cast(lpar_dma_control), int_cast(lpar_driver_info), int_cast(lpar_reports), mem_handle /*0x5a5a5a5b*/, 0x820);
	return_to_user_prog(u32);
}

u32 sys_rsx_context_attribute(u32 context_id, u32 package_id, u64 a3, u64 a4, u64 a5, u64 a6)
{
	system_call_6(SYS_RSX_CONTEXT_ATTRIBUTE, context_id, package_id, a3, a4, a5, a6);
	return_to_user_prog(u32);	
}

u32 sys_rsx_device_map(u64* dev_addr)
{
	system_call_3(SYS_RSX_DEVICE_MAP, int_cast(dev_addr), 0, 8);
	return_to_user_prog(u32);
}

u32 sys_rsx_attribute(u32 pkg_id, u32 a2, u32 a3, u32 a4, u32 a5)
{
	system_call_5(SYS_RSX_ATTRIBUTE, pkg_id, a2, a3, a4, a5);
	return_to_user_prog(u32);	
}

template <typename T>
u32 sys_rsx_context_iomap(u32 context_id, u32 io, T ea, u32 size, u64 flags)
{
	system_call_5(SYS_RSX_CONTEXT_IOMAP, context_id, io, int_cast(reinterpret_cast<const void*>(ea)), size, flags);
	return_to_user_prog(u32);
}

template <typename T>
u32 GcmMapEaIoAddress(T ea, u32 io, u32 size)
{
	return cellGcmMapEaIoAddress(reinterpret_cast<const void*>(ea), io, size);
}

u32 sys_rsx_context_iounmap(u32 context_id, u32 io, u32 size)
{
	system_call_3(SYS_RSX_CONTEXT_IOUNMAP, context_id, io, size);
	return_to_user_prog(u32);
}

// Get rsx context, if there are more than one use the one ordered by index
u32 get_sys_rsx_context(u32 index = 0)
{
	if (index > 3)
	{
		// Max context count per process
		return 0;
	}

	u32 ctxt_id = 0x55555550;
	const u32 max = ctxt_id + 0x20; // TODO: Check this
	u32 ret = EINVAL;

	for (u32 i = 0; ctxt_id < max; ctxt_id++)
	{
		// Ultra hack
		if (sys_rsx_context_iounmap(ctxt_id, 255<<20, 1<<20))
		{
			continue;
		}

		if (i++ == index)
		{
			return ctxt_id;
		}

		if (i > index)
		{
			return 0;
		}
	}

	return 0;
}
#endif