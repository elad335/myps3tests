Assemble spu code: 

```
spu-lv2-gcc -o test_spu.spu.out cell-spu.s test_runner.spu.c
```

Convert spu to embeddable ppu image: 
```
ppu-lv2-objcopy -I binary -O elf64-powerpc-celloslv2 -B powerpc \
  --set-section-align .data=7 --set-section-pad .data=128 \
  --rename-section .data=.spu_image.test,readonly,contents,alloc \
  test_spu.spu.out test_spu.o
```

Create final executable ppu elf
```
ppu-lv2-gcc -o test_spu.ppu.elf test_spu.o test_runner.ppu.c
```

Elf uses `stopd` when there is failed instructions to avoid exit. This should allow easier debugging

Make file creation is left as an exercise for the reader

Todo: Absolute branching, halt, stop and invalid channel are all not currently tested in cell-spu
see cell-spu.s for instructions on what would be needed to test those
`TEST_CHANNEL_INVALID` was moved to encompass more channel checks that emulator currently doesnt support fully and will cause a crash