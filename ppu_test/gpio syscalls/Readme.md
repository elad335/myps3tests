Create final executable ppu elf
```
ppu-lv2-gcc -o test_ppu.elf cell-ppu.s test_runner.c
```

Todo: Syscall, trap and absolute branching is disabled, see cell-ppu.s for enabling and how to test them
'custom' opcodes are commented out, not because they are wrong, but for ease of testing and implementation for now
```
0x7C694492
0x7C694496
0x7C694412
0x7C694416
```
'rc' instructions are commented, frsp. fadd. fadds. fsub. fsubs. fmul. fmuls. fdiv. fdivs. fmadd. fmadds. fmsub. fmsubs. fnmadd. fnmadds. fnmsub. fnmsubs. fctid. fctidz. fctiw. fcfid. fctiwz. fsqrt. fsqrts. fres. frsqrte. fsel.

Code throws assert if invalid instructions are detected at end to aide in debugging