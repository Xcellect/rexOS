# Purpose: Set the stack pointer since 
# 1. the bootloader does not set the stack pointer
# 2. the kernel.cpp expects it to be set

# the following is put in the compiler variable
.set MAGIC, 0x1badb002 			# to make the bootloader believe this is a kernel
.set FLAGS, (1<<0 | 1<<1) # for the bootloader
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot			# putting these in the .o file
	.long MAGIC				# just these variables
	.long FLAGS
	.long CHECKSUM

# text segment of the code
.section .text
# loader.o (<-loader.s) will be linked to kernel.o(<-kernel.cpp) 
# so here we declare there will kernelMain
.extern kernelMain
.extern callConstructors


# Program entry point
.global loader
loader:
	mov $kernel_stack, %esp		# set the stack ptr to ptr for some stack (declared in bss)
	call callConstructors
	# before the bootloader loads the kernel it creates a multiboot structure in the RAM.
	# in the following instructions we're building the main function's arg array 
	# 1. taking multiboot struct ptr in ax 
	# 2. the multiboot magic number in the bx
	push %eax		# take the multiboot struct ptr
	push %ebx		# take the magic number
	call kernelMain

_stop:							# infinite loop
	cli
	hlt
	jmp _stop




.section .bss
.space 2*1024*1024;	# 2 MiB space padding after text segment so stack doesn't overwrite text

kernel_stack:
