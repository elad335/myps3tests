#include <sys/spu_thread.h>	
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <spu_internals.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

inline void sync() {asm volatile("sync;syncc;dsync");};

int main(void)
{
	spu_writech(SPU_WrOutIntrMbox, 0); // Trigger interrupt
	sync();
	return 0;
}
