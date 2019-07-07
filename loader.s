.set MAGIC, 0x1badb002 	# to make the bootloader believe this is a kernel
.set FLAGS, (1<<0 | 1<<1)
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
	.long MAGIC
	.long FLAGS
	.long CHECKSUM


.section .text
# Declare external functions (linked to the kernel.cpp in the linker.ld)
.extern kernelMain
.extern callConstructors
.global loader

loader:
	mov $kernel_stack, %esp
	call callConstructors
	push %eax
	push %ebx
	call kernelMain

_stop:
	cli
	hlt
	jmp _stop




.section .bss
.space 2*1024*1024;	# 2 MiB space padding between stack and text segments so stack doesn't overwrite text

kernel_stack:
