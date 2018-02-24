spu-lv2-gcc -o test_spu.spu.out test_runner.spu.cpp

ppu-lv2-objcopy -I binary -O elf64-powerpc-celloslv2 -B powerpc --set-section-align .data=7 --set-section-pad .data=128 --rename-section .data=.spu_image.test,share,contents,alloc test_spu.spu.out test_spu.o

ppu-lv2-gcc -o test_spu.ppu.elf test_spu.o test_runner.ppu.cpp

make_fself test_spu.ppu.elf test_spu.ppu.self 

pause