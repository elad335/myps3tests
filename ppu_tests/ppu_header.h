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

#ifndef ENSURE_OK

s32 ensure_ok_ret_save_ = 0; // Unique naming
#define ENSURE_OK(x) if ((ensure_ok_ret_save_ = s32(x)) != CELL_OK) { printf("\"%s\" Failed at line %d! (error=0x%x)", #x, __LINE__, ensure_ok_ret_save_); exit(-1); }

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
static const volatile T as_volatile_v = value;
#endif

#ifndef cellFunc

// TODO
s64 g_ec = 0;
#define cellFunc(name, ...) \
(printf("cell" #name "(error=0x%x)\n", u32(g_ec = cell##name(__VA_ARGS__))), g_ec)
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
