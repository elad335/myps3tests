#include <sys/spu_thread.h>
#include <spu_mfcio_gcc.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>
#include <sys/sys_fixed_addr.h>
#include <stdint.h>
#include <cstring>
#include <string>

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

typedef float f32;
typedef double f64;

typedef vec_ushort8 vec_u16;
typedef vec_uchar16 vec_u8;
typedef vec_uint4 vec_u32;
typedef vec_ullong2 vec_u64;
typedef vec_short8 vec_s16;
typedef vec_char16 vec_s8;
typedef vec_int4 vec_s32;
typedef vec_llong2 vec_s64;
typedef vec_float4 vec_f32;
typedef vec_double2 vec_f64;

#define ptr_cast(x) reinterpret_cast<void*>(x)
#define ptr_caste(x, T) reinterpret_cast<T*>(ptr_cast(x))
#define ref_cast(x, T) *reinterpret_cast<T*>(ptr_cast(x))
#define int_cast(x) reinterpret_cast<uptr>(x)

// Lazy memory barrier
#ifndef fsync
#define fsync() ({ __asm__ volatile ("syncc" : : : "memory"); })

//template <typename T>
//volatile T& as_volatile(T& obj)
//{
//	fsync();
//	return const_cast<volatile T&>(obj);
//}

//template <typename T>
//const volatile T& as_volatile(const T& obj)
//{
//	fsync();
//	return const_cast<const volatile T&>(obj);
//}

template <typename T, T value>
static const volatile T& as_volatile_v()
{
	static const volatile T v = value;
	return v;
}

#endif

void print_bytes(const volatile void* data, size_t size)
{
	const volatile u8* bytes = static_cast<const volatile u8*>(data);

	for (u32 i = 0; i < size;)
	{
		if (size > 0xfff)
		{
			spu_printf("[%04x] |", i);
		}
		else
		{
			spu_printf("[%03x] |", i);
		}
		

		switch (size - i)
		{
		default: spu_printf(" %02X", bytes[i++]);
		case 7: spu_printf(" %02X", bytes[i++]);
		case 6: spu_printf(" %02X", bytes[i++]);
		case 5: spu_printf(" %02X |", bytes[i++]);
		case 4: spu_printf(" %02X", bytes[i++]);
		case 3: spu_printf(" %02X", bytes[i++]);
		case 2: spu_printf(" %02X", bytes[i++]);
		case 1: spu_printf(" %02X |", bytes[i++]);
		}

		spu_printf("\n");
	}
}

template <typename T>
void print_obj(const volatile T& obj)
{
	print_bytes(&obj, sizeof(obj));
}

template <typename T>
void reset_obj(T& obj, int ch = 0)
{
	std::memset(&obj, ch, sizeof(obj));
}

template <typename To, typename From>
static inline To bit_cast(const From& from)
{
	To to;
	std::memcpy(&to, &from, sizeof(From));
	return to;
}

inline u32 mfcsync(u32 mask, u32 type = MFC_TAG_UPDATE_ALL) 
{
	if (mfc_stat_tag_status())
	{
		mfc_read_tag_status();
	}

	mfc_write_tag_mask(mask);
	mfc_write_tag_update(type);
	const u32 res = mfc_read_tag_status();
	fsync();
	return res;
};

// Dont use tag update requests if we want to wait for all MFC commands
inline void mfcsync() 
{
	while (16 - mfc_stat_cmd_queue())
	{
		fsync();
	}

	fsync();
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

	void* data(u32 offset = 0)
	{
		return rdata + offset;
	}

	uptr addr() const
	{
		return int_cast(+rdata);
	}
};

// Workaround for broken X86 MULPD instruction (INFINITY isn't formed well for 0*INIFNITY expression to work) 
static const f32 const_nan = bit_cast<f32>(0x7fc00000);

#define RAW_SPU_OFFSET        0x00100000UL
#define RAW_SPU_LS_OFFSET     0x00000000UL
#define RAW_SPU_PROB_OFFSET   0x00040000UL

#define MFC_Size_Tag      0x3010U
#define MFC_Class_CMD     0x3014U
#define MFC_CMDStatus     0x3014U
#define MFC_QStatus       0x3104U
#define Prxy_QueryType    0x3204U
#define Prxy_QueryMask    0x321CU
#define Prxy_TagStatus    0x322CU
#define SPU_Out_MBox      0x4004U
#define SPU_In_MBox       0x400CU
#define SPU_MBox_Status   0x4014U
#define SPU_RunCntl       0x401CU
#define SPU_Status        0x4024U
#define SPU_NPC           0x4034U
#define SPU_Sig_Notify_1  0x1400CU
#define SPU_Sig_Notify_2  0x1C00CU

/* Macros to calculate base addresses with the given Raw SPU ID */
#define LS_BASE_ADDR(id) \
(RAW_SPU_OFFSET * id + RAW_SPU_BASE_ADDR + RAW_SPU_LS_OFFSET)
#define PROB_BASE_ADDR(id) \
(RAW_SPU_OFFSET * id + RAW_SPU_BASE_ADDR + RAW_SPU_PROB_OFFSET)


#define get_ls_addr(id, offset) \
(volatile uintptr_t)(LS_BASE_ADDR(id) + offset)
#define get_reg_addr(id, offset) \
(volatile uintptr_t)(PROB_BASE_ADDR(id) + offset)

extern inline void
sys_raw_spu_mmio_write_ls(int id, int offset, uint32_t value);

extern inline void sys_raw_spu_mmio_write_ls(int id, int offset, uint32_t value)
{
	*(volatile uint32_t *)get_ls_addr(id, offset) = value;
}

extern inline uint32_t sys_raw_spu_mmio_read_ls(int id, int offset);

extern inline uint32_t sys_raw_spu_mmio_read_ls(int id, int offset)
{
	return *(volatile uint32_t *)get_ls_addr(id, offset);
}

extern inline void sys_raw_spu_mmio_write(int id, int offset, uint32_t value);

extern inline void sys_raw_spu_mmio_write(int id, int offset, uint32_t value)
{
	*(volatile uint32_t *)get_reg_addr(id, offset) = value;
}

extern inline uint32_t sys_raw_spu_mmio_read(int id, int offset);

extern inline uint32_t sys_raw_spu_mmio_read(int id, int offset)
{
	return *(volatile uint32_t *)get_reg_addr(id, offset);
}

#define spu_op1_f(inst, arg1) si_to_float(si_##inst(si_from_float(arg1)))
#define spu_op2_f(inst, arg1, arg2) si_to_float(si_##inst(si_from_float(arg1), si_from_float(arg2)))
#define spu_op3_f(inst, arg1, arg2, arg3) si_to_float(si_##inst(si_from_float(arg_), si_from_float(arg2), si_from_float(arg3)))

#define spu_op1_d(inst, arg1) si_to_double(si_##inst(si_from_double(arg1)))
#define spu_op2_d(inst, arg1, arg2) si_to_double(si_##inst(si_from_double(arg1), si_from_double(arg2)))
#define spu_op3_d(inst, arg1, arg2, arg3) si_to_double(si_##inst(si_from_double(arg1), si_from_double(arg2), si_from_double(arg3)))

// Runtime(!) channel interaction functions

static u32 read_count(u32 ch)
{
	static u32 f[] __attribute__ ((aligned(16))) =
	{
		0x01E00003, // rchcnt $3, $ch(?)
		0x35000000, // bi $LR
		0x4020007F, // lnop
	};

	f[0] &= ~(0x7F << 7);
	f[0] |= (ch & 0x7F) << 7;
	fsync();
	return ((u32(*)(void))(uptr)f)();
}
 
static u32 read_ch(u32 ch)
{
	static u32 f[] __attribute__ ((aligned(16))) =
	{
		0x01A00003, // rdch $3, $ch(?)
		0x35000000, // bi $LR
		0x4020007F, // lnop
	};

	f[0] &= ~(0x7F << 7);
	f[0] |= (ch & 0x7F) << 7;
	fsync();
	return ((u32(*)(void))(uptr)f)();
}
 
void write_ch(u32 ch, u32 value)
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
	fsync();
	((void(*)(u32))(uptr)f)(value);
}
