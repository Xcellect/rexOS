# Purpose: Set the stack pointer since 
# 1. the bootloader does not set the stack pointer
# 2. the kernel.cpp expects it to be set
# This is where the kernel space is built. So all the processes start here.

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
# .extern declares external functions from other object files this file's object
# is combined with. it's like function declaration in C/C++
.extern kernelMain			# main function of kernel.o
.extern callConstructors	# common constructor function


# Program entry point
.global loader
loader:
	mov $kernel_stack, %esp		# set the stack ptr to ptr for some stack (declared in bss)
	call callConstructors		# (K.1.) build the kernel stack space
	# before the bootloader loads the kernel it creates a multiboot structure in the RAM.
	# in the following instructions we're building the main function's arg array 
	# 1. taking multiboot struct ptr in ax 
	# 2. the multiboot magic number in the bx
	push %eax		# push multiboot struct ptr
	push %ebx		# push multiboot magic number
	call kernelMain	# (K.2.) call kernel.o's main function

_stop:							# infinite loop
	cli
	hlt
	jmp _stop



# bss segment is for undeclared data
.section .bss
.space 4*1024*1024;	# 4 MiB stack space

kernel_stack:
