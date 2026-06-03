process_1:
	i686-elf-gcc -m32 -ffreestanding -nostdlib -g -c -O0 process_1.c -o process_1.o
	i686-elf-ld -T process.ld process_1.o -o process_1.elf
	i686-elf-objcopy -O binary process_1.elf process_1.bin

process_2:
	i686-elf-gcc -m32 -ffreestanding -nostdlib -g -c -O0 process_2.c -o process_2.o
	i686-elf-ld -T process.ld process_2.o -o process_2.elf
	i686-elf-objcopy -O binary process_2.elf process_2.bin