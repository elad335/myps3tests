
#include <sys/prx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/tty.h>
#include <sys/synchronization.h>

//#include "../ppu_header.h"
SYS_MODULE_INFO(hello, 0, 1, 1);
SYS_MODULE_START(prx_entry);
SYS_MODULE_STOP(prx_stop);
SYS_MODULE_EXIT(prx_stop);

SYS_LIB_DECLARE( EladPrxLibrary, SYS_LIB_AUTO_EXPORT | SYS_LIB_WEAK_IMPORT );
SYS_LIB_EXPORT( prx_function_elad, EladPrxLibrary );

static volatile int s_buffer[4];

void write_tty(const char* p)
{
    unsigned int out = 0;
    sys_tty_write(SYS_TTYP_PPU_STDOUT, p, strlen(p), &out);
}
extern "C" int prx_entry(unsigned int args, void* argp)
{
    char p[] = "PRX entry start!: \n";
    unsigned int out = 0;
    sys_tty_write(SYS_TTYP_PPU_STDOUT,   p, sizeof("PRX entry start!\n"), &out);

    char text[32/4] = {};

    for (int i = 28; i >= 0; i-=4)
    {
        text[i / 4] = "0123456789ABCEDF"[(((unsigned int)(+s_buffer)) >> i) % 16];
    }

    write_tty(text);
    
    return SYS_PRX_RESIDENT;
}

extern "C" int prx_stop()
{
    const char* p = "PRX entry stop!\n";
    unsigned int out = 0;
    sys_tty_write(SYS_TTYP_PPU_STDOUT,   p, sizeof("PRX entry stop!\n"), &out);
    return 0;
}


extern "C" unsigned long long prx_function_elad()
{
    const char* p = "PRX entry stop!\n";
    unsigned int out = 0;
    //printf("PRX function at %p", +s_buffer);
    //sys_prx_get_my_module_id();
    char text[32/4 + 1] = {};
    text[32/4] = '\n';

    for (int i = 28; i >= 0; i-=4)
    {
        text[i / 4] = "0123456789ABCEDF"[(((unsigned int)(+s_buffer)) >> i) % 16];
    }

    write_tty(text);
    
    return (unsigned int)(+s_buffer);
}

