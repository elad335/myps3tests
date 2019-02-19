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

#define mfence() asm volatile ("eieio;sync")

inline u32 lv2_lwcond_wait(sys_lwcond_t* lwc, sys_lwmutex_t* mtx, u64 timeout)
{
	system_call_3(0x071, *ptr_caste(int_cast(lwc) + 4, u32), *ptr_caste(int_cast(mtx) + 16, u32), timeout);
	return_to_user_prog(u32);
}

inline u32 lv2_lwcond_signal(sys_lwcond_t* lwc, sys_lwmutex_t* mtx, u32 ppu_id, u32 mode)
{
	system_call_4(0x073, *ptr_caste(int_cast(lwc) + 4, u32), *ptr_caste(int_cast(mtx) + 16, u32), ppu_id, mode);
	return_to_user_prog(u32);
}

inline u32 lv2_lwcond_signal_all(sys_lwcond_t* lwc, sys_lwmutex_t* mtx, u32 mode)
{
	system_call_3(0x074, *ptr_caste(int_cast(lwc) + 4, u32), *ptr_caste(int_cast(mtx) + 16, u32), mode);
	return_to_user_prog(u32);
}

inline u32 lv2_lwmutex_lock(sys_lwmutex_t* mtx, u64 timeout)
{
	system_call_2(0x061, *ptr_caste(int_cast(mtx) + 16, u32), timeout);
	return_to_user_prog(u32);
}

inline u32 lv2_lwmutex_trylock(sys_lwmutex_t* mtx)
{
	system_call_1(0x063, *ptr_caste(int_cast(mtx) + 16, u32));
	return_to_user_prog(u32);
}

inline u32 lv2_lwmutex_unlock(sys_lwmutex_t* mtx)
{
	system_call_1(0x062, *ptr_caste(int_cast(mtx) + 16, u32));
	return_to_user_prog(u32);
}