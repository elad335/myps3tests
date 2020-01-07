
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ppu_thread.h> 
#include <sys/spu_thread.h>
#include <sys/raw_spu.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>
#include <sys/interrupt.h>
#include <sys/paths.h>
#include <sys/process.h>
#include <spu_printf.h>
#include <sysutil/sysutil_sysparam.h>  
#include <sys/timer.h>
#include <spu_printf.h>
#include <sys/raw_spu.h>
#include "../../../ppu_tests/ppu_header.h"

/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];

static sys_raw_spu_t thr_id = ~0x0;

enum
{
	MFC_PUT_CMD      = 0x20, MFC_PUTB_CMD     = 0x21, MFC_PUTF_CMD     = 0x22,
	MFC_PUTS_CMD     = 0x28, MFC_PUTBS_CMD    = 0x29, MFC_PUTFS_CMD    = 0x2a,
	MFC_PUTR_CMD     = 0x30, MFC_PUTRB_CMD    = 0x31, MFC_PUTRF_CMD    = 0x32,
	MFC_GET_CMD      = 0x40, MFC_GETB_CMD     = 0x41, MFC_GETF_CMD     = 0x42,
	MFC_GETS_CMD     = 0x48, MFC_GETBS_CMD    = 0x49, MFC_GETFS_CMD    = 0x4a,
	MFC_PUTL_CMD     = 0x24, MFC_PUTLB_CMD    = 0x25, MFC_PUTLF_CMD    = 0x26,
	MFC_PUTRL_CMD    = 0x34, MFC_PUTRLB_CMD   = 0x35, MFC_PUTRLF_CMD   = 0x36,
	MFC_GETL_CMD     = 0x44, MFC_GETLB_CMD    = 0x45, MFC_GETLF_CMD    = 0x46,
	MFC_GETLLAR_CMD  = 0xD0,
	MFC_PUTLLC_CMD   = 0xB4,
	MFC_PUTLLUC_CMD  = 0xB0,
	MFC_PUTQLLUC_CMD = 0xB8,

	MFC_SNDSIG_CMD   = 0xA0, MFC_SNDSIGB_CMD  = 0xA1, MFC_SNDSIGF_CMD  = 0xA2,
	MFC_BARRIER_CMD  = 0xC0,
	MFC_EIEIO_CMD    = 0xC8,
	MFC_SYNC_CMD     = 0xCC,

	MFC_BARRIER_MASK = 0x01,
	MFC_FENCE_MASK   = 0x02,
	MFC_LIST_MASK    = 0x04,
	MFC_START_MASK   = 0x08,
	MFC_RESULT_MASK  = 0x10, // ???
	MFC_PROXY_COMMAND_QUEUE_EMPTY_FLAG = 1u << 31,
};

#define DMA_LSA       MFC_LSA      
#define DMA_EAH       MFC_EAH      
#define DMA_EAL       MFC_EAL      
#define DMA_Size_Tag  MFC_Size_Tag 
#define DMA_Class_CMD MFC_Class_CMD
#define DMA_CMDStatus MFC_CMDStatus
#define DMA_QStatus   MFC_QStatus  
#define PU_MB         SPU_Out_MBox
#define SPU_MB        SPU_In_MBox
#define MB_Stat       SPU_MBox_Status

#define mmio_write_prob_reg(reg, x) sys_raw_spu_mmio_write(thr_id, reg, (x))
#define mmio_read_prob_reg(x) sys_raw_spu_mmio_read(thr_id, (x))

static u8 buf[0x4000];

void mfc_proxy_wait()
{
	fsync();
	while (!(mmio_read_prob_reg(DMA_QStatus) & MFC_PROXY_COMMAND_QUEUE_EMPTY_FLAG))
	{
		sys_timer_usleep(5);
	}
	fsync();
}

// Write with no cmd
void mfc_proxy_args0(u32 lsa, u16 size)
{
	fsync();
	mmio_write_prob_reg(DMA_LSA, lsa);
	mmio_write_prob_reg(DMA_Size_Tag, ((size + 0u) << 16)); // Tag is 0
	mmio_write_prob_reg(DMA_EAH, 0);
	mmio_write_prob_reg(DMA_EAL, int_cast(+buf));
	fsync();
}

void mfc_proxy_args(u32 cmd, u32 lsa, u16 size)
{
	mfc_proxy_args0(lsa, size);
	mmio_write_prob_reg(DMA_Class_CMD, cmd);
	fsync();
}

void buf_memset(void* ptr, int val, u16 size)
{
	fsync();
	volatile u8* data = static_cast<u8*>(ptr);
	for (u32 i = 0; i < size; i++)
	{
		*(data++) = static_cast<u8>(val);
	}
	fsync();
}

void mfc_memset(u32 lsa, int val, u16 size)
{
	buf_memset(buf, val, size);
	mfc_proxy_args(MFC_PUTB_CMD, 0, size);
	ENSURE_OK(mmio_read_prob_reg(DMA_CMDStatus));
	mfc_proxy_wait();
}

u32 mfc_load_byte(u32 lsa)
{
	const u8 val = as_volatile(*(u8*)get_ls_addr(thr_id, lsa));
	return val;
}

void mfc_store_byte(u32 lsa, u8 val)
{
	as_volatile(*(u8*)get_ls_addr(thr_id, lsa)) = val;
	fsync();
}

int main(void)
{
	ENSURE_OK(sys_spu_initialize(6, 1)); // 1 raw threads max
	ENSURE_OK(sys_raw_spu_create(&thr_id, NULL));

	struct 
	{
		u8 put_val;
		u8 get_val;
		u8 last_val;
		u8 after_fail_cmd_res;
	} result = {};

	// 0x00 vs 0xff
	mfc_store_byte(0, 0);
	buf_memset(buf, 0xff, 1);

	mfc_proxy_args(MFC_GETB_CMD, 0, 1);
	mfc_proxy_wait();

	result.put_val = mfc_load_byte(0); 
	buf_memset(buf, 0x80, 1);
	mfc_proxy_args(MFC_PUTB_CMD, 0, 1);
	mfc_proxy_wait();
	result.get_val = as_volatile(buf[0]); 

	// Read command status
	ENSURE_OK(mmio_read_prob_reg(DMA_CMDStatus));
	mfc_proxy_wait();

	// If PUT was executed first value is 0xff
	// Otherwise 0x0
	result.last_val = as_volatile(buf[0]);

	mmio_write_prob_reg(DMA_Class_CMD, MFC_PUT_CMD);
	fsync();
	/*ENSURE_OK*/(mmio_read_prob_reg(DMA_CMDStatus) == 0); // Must fail (no assertation for it to work on rpcs3)
	mfc_proxy_args0(0, 1);
	result.after_fail_cmd_res = mmio_read_prob_reg(DMA_CMDStatus);
	mfc_proxy_wait();

	printf("RAWSPU MMIO result:\n"
	"SPU buffer value after GETB = 0x%02x\n"
	"PPU buffer value after PUTB = 0x%02x\n"
	"PPU buffer after reading CMDStatus: = 0x%02x\n"
	"DMA written after failure status = 0x%x\n"
	, result.put_val
	, result.get_val
	, result.last_val
	, result.after_fail_cmd_res
	);

	return 0;
}

