BUILD?=build
ASM?=nasm

$(PHONY): build clean

build:
	$(ASM) -f elf64 modules/IDT/IDT.asm -o $(BUILD)/IDT.o
	/usr/local/x86_64elfgcc/bin/x86_64-elf-g++ -Ttext 0x8000 -ffreestanding -mno-red-zone -m64 -fpermissive -I . -Wwrite-strings -c "main.cpp" -o "$(BUILD)/main.o"
	/usr/local/x86_64elfgcc/bin/x86_64-elf-ld -T"linker.ld" $(BUILD)/stage2.o $(BUILD)/main.o $(BUILD)/IDT.o

clean:
	rm -rf $(BUILD)/main.o
	rm -rf $(BUILD)/kernel.bin
	rm -rf $(BUILD)/IDT.o
	rm -rf $(BUILD)/API.o