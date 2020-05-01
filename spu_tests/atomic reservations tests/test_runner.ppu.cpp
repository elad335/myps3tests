
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/spu_thread_group.h>
#include <sys/spu_thread.h>
#include <sys/spu_utility.h>
#include <sys/spu_image.h>
#include <sys/spu_initialize.h>
#include <string>
#include <spu_printf.h>
#include <sys/timer.h>

#include "../../ppu_tests/ppu_header.h"

/* embedded SPU ELF symbols */
extern char _binary_test_spu_spu_out_start[];

static u8 rdata[1024] ALIGN(128) = {0};

struct thread_data_t
{
	u32 signal;
	void(*cb)(thread_data_t&);
	u32 exit;
	u32 addr;
};

// Thread which touches reservation data
void helper_thread(u64 addr)
{
	thread_data_t& data = *ptr_caste(addr, thread_data_t);

	while (true)
	{
		while (!load_vol(data.signal))
		{
			sys_ppu_thread_yield();
		}

		if (load_vol(data.exit))
		{
			break;
		}

		load_vol(data.cb)(data);
		store_vol(data.signal, 0);
	}

	thread_exit(0);
}

// Execute callback on the other thread
void wrap_callback(thread_data_t& data, void(*cb)(thread_data_t&))
{
	store_vol(data.cb, cb);
	store_vol(data.signal, 1);

	while (load_vol(data.signal)) __db16cyc();
}

// Callbacks executed on two different ppu threads

// Change reservation address within the same cache line
void change_addr(thread_data_t& data)
{
	store_vol(data.addr, load_vol(data.addr) ^ 0x78);
}

// Write value to the reservation cache line from different address
void write_val(thread_data_t& data)
{
	store_vol(*ptr_caste(load_vol(data.addr) ^ 0x78, u32), 0); 
}

// Execut full atomic op once on reservation cache line from different address
void atomic_op(thread_data_t& data)
{
	void* ptr = ptr_cast(load_vol(data.addr) ^ 0x78);
	u32 old;

	do
	{
		old = __lwarx(ptr);
	}
	while (0 == __stwcx(ptr, old));
}

// Execute a single store reservation instruction
void store_cond(thread_data_t& data)
{
	u32* ptr = ptr_caste(load_vol(data.addr) ^ 0x78, u32);
	__stwcx(ptr, load_vol(*ptr)); 
}

void invalidate_cache(thread_data_t& data)
{
	void* rdata = ptr_cast(load_vol(data.addr));
	__dcbst(rdata);
	__dcbtst(rdata);
	__dcbf(rdata);
}

void ppu_reservation_tests()
{
	printf("PPU tests on self thread:\n");

	reset_obj(rdata);

	sys_ppu_thread_t help = 0;
	thread_data_t data = {};

	const struct callback_info_t
	{
		void(*cb)(thread_data_t&);
		const char* name;
	} calls[5] =
	{
		{&change_addr, "change_addr"},
		{&write_val, "write_val"},
		{&invalidate_cache, "invalidate_cache"},
		{&atomic_op, "atomic_op"},
		{&store_cond, "store_cond"}
	};

	ENSURE_OK(sys_ppu_thread_create(&help, helper_thread,bit_cast<uptr>(&data),1001, 0x10000,1,"t"));

	static const u32 to_repeat = 500; 

	// Testing reservation modification with different instruction mashups
	for (u32 i = 0; i < 4; i++)
	{
		bool success = false;

		for (u32 j = 0; j < to_repeat && !success; sys_ppu_thread_yield(), j++)
		{
			(i & 1) ? __ldarx(rdata) : __lwarx(rdata);

			// In case we lost reservation due to context switch, repeat a few time
			success |= ((i & 2) ? __stdcx(rdata, 0) : __stwcx(rdata, 0)) != 0;
		}

		printf("%s load and %s store, success: %d\n", (i & 1) ? "LDARX" : "LWARX", (i & 2) ? "STDCX" : "STWCX", +success);
	}

	for (u32 i = 0; i < 5/*std::size(calls)*/; i++)
	{
		bool success = false;

		for (u32 j = 0; j < to_repeat && !success; sys_ppu_thread_yield(), j++)
		{
			store_vol(data.addr, int_cast(&rdata));

			__lwarx(rdata);

			calls[i].cb(data);

			// In case we lost reservation due to context switch, repeat a few time
			success |= __stwcx(ptr_cast(load_vol(data.addr)), 0);
		}

		printf("Function '%s' executed on reservation, success: %d\n", calls[i].name, +success);
	}

	printf("PPU tests with another thread:\n");

	for (u32 i = 0; i < 5/*std::size(calls)*/; i++)
	{
		bool success = false;

		for (u32 j = 0; j < to_repeat && !success; sys_ppu_thread_yield(), j++)
		{
			store_vol(data.addr, int_cast(&rdata));

			__lwarx(rdata);

			wrap_callback(data, calls[i].cb);

			// In case we lost reservation due to context switch, repeat a few time
			success |= __stwcx(ptr_cast(load_vol(data.addr)), 0);
		}

		printf("Function '%s' executed asynchronously on reservation, success: %d\n", calls[i].name, +success);
	}

	store_vol(data.exit, 1);
	store_vol(data.signal, 1);
	ENSURE_VAL(sys_ppu_thread_join(help, NULL), EFAULT);
	sys_timer_sleep(1);
} 

int main(void)
{
	ppu_reservation_tests();

	sys_spu_initialize(1, 0);

	sys_spu_thread_group_t grp_id;
	int prio = 100;
	sys_spu_thread_group_attribute_t grp_attr;
	
	sys_spu_thread_group_attribute_initialize(grp_attr);
	sys_spu_thread_group_attribute_name(grp_attr, "test spu grp");
	ENSURE_OK(sys_spu_thread_group_create(&grp_id, 2, prio, &grp_attr));

	sys_spu_image_t img;
	ENSURE_OK(sys_spu_image_import(&img, (void*)_binary_test_spu_spu_out_start, SYS_SPU_IMAGE_DIRECT));

	sys_spu_thread_t thr_id;
	sys_spu_thread_attribute_t thr_attr;
	
	sys_spu_thread_attribute_initialize(thr_attr);
	sys_spu_thread_attribute_name(thr_attr, "test spu thread");
	sys_spu_thread_argument_t thr_args;
	thr_args.arg1 = reinterpret_cast<uptr>(&rdata[0]);
	thr_args.arg2 = 0;
	ENSURE_OK(sys_spu_thread_initialize(&thr_id, grp_id, 0, &img, &thr_attr, &thr_args));

	thr_args.arg2 = 1;
	ENSURE_OK(sys_spu_thread_initialize(&thr_id, grp_id, 1, &img, &thr_attr, &thr_args));

	ENSURE_OK(spu_printf_initialize(1000, NULL));
	ENSURE_OK(spu_printf_attach_group(grp_id));

	ENSURE_OK(sys_spu_thread_group_start(grp_id));

	int cause;
	int status;
	ENSURE_OK(sys_spu_thread_group_join(grp_id, &cause, &status));

	spu_printf_detach_group(grp_id);
	spu_printf_finalize();

	ENSURE_OK(sys_spu_thread_group_destroy(grp_id));

	sys_timer_sleep(1);
	printf("sample finished");
	return 0;
}
