#include <sys/spu_thread.h>
#include <spu_mfcio_gcc.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t uptr;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef intptr_t sptr;

#define ptr_cast(x) reinterpret_cast<void*>(x)
#define ptr_caste(x, T) reinterpret_cast<T*>(ptr_cast(x))
#define ref_cast(x, T) *reinterpret_cast<T*>(ptr_cast(x))
#define int_cast(x) reinterpret_cast<uptr>(x)

inline void mfence() { asm volatile ("syncc;sync;dsync"); };

inline void mfc_fence() 
{
    spu_writech(MFC_WrTagMask, -1u);
    spu_writech(MFC_WrTagUpdate, 2); 
    si_rdch(MFC_RdTagStat);
    mfence();
};

struct lock_line_t
{
	u8 rdata[128] __attribute__((aligned(128)));

	// Access reservation data as array 
	template<typename Ts>
	Ts& as(std::size_t index)
	{
	    return ptr_caste(rdata, Ts)[index];
	}

	u32& operator [](std::size_t index)
	{
		return ptr_caste(rdata, u32)[index];
	}

	void* data()
	{
		return &rdata[0];
	}

	uptr addr()
	{
		return int_cast(&rdata[0]);
	}
};

// Runtime(!) channel interaction functions

/*
static u32 get_channel_count(u32 ch)
{
	static u32 f[] __attribute__ ((aligned(16))) =
    {
		0x01E00003, // rchcnt $3, $ch(?)
		0x35000000, // bi $LR
		0x4020007F, // lnop
	};

	f[0] &= ~(0x7F << 7);
	f[0] |= (ch & 0x7F) << 7;
	si_sync();
	return ptr_caste(&f, u32(*)(void))();
}
 
static u32 read_channel(u32 ch)
{
	static u32 f[] __attribute__ ((aligned(16))) =
    {
		0x01A00003, // rdch $3, $ch(?)
		0x35000000, // bi $LR
		0x4020007F, // lnop
	};

	f[0] &= ~(0x7F << 7);
	f[0] |= (ch & 0x7F) << 7;
	si_sync();
	return ptr_caste(&f, u32(*)(void))();
}
 
void write_channel(u32 ch, u32 value)
{
	static u32 f[] __attribute__ ((aligned(16))) =
    {
		0x21A00003, // wrch $ch(?), $3
        0x00500000, // syncc
		0x35000000, // bi $LR
		0x4020007F, // lnop
	};

	f[0] &= ~(0x7F << 7);
	f[0] |= (ch & 0x7F) << 7;
	void(* pf)(u32) = ptr_caste(&f[0], void(*)(u32));
	si_sync();
	pf(value);
}
*/
