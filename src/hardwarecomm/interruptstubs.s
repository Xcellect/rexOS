# (I.1.)
# Keyboard interrupt is 0x1. This is also an internal interrupt from the CPU.
# That's why an offset of 0x20 is added with each received interrupt here.
.set IRQ_BASE, 0x20	# IRQ_BASE (compiler variable) is set to 32 or 0x20

.section .text
# These markers/landing points won't work after changing namespace
# constructed as following:
# 1. starts with _ZN
# 2. then class/namespace/method names (eg. InterruptManager)
# 3. length of each name precedes it
# 4. finally the parameters for the method (eg. Ehj, Ev)

# (I.3.)
# Following is an external function from a different object (cpp) file. Declared 
# like a function declaration on C/C++ (eg. the printf function defined in  
# kernel is declared whenever a different code file needed to use it)
.extern _ZN5rexos12hardwarecomm16InterruptManager15handleInterruptEhj



# Start Macro 2: HandleException
.macro HandleException num
.global _ZN5rexos12hardwarecomm16InterruptManager16HandleException\num\()Ev

_ZN5rexos12hardwarecomm16InterruptManager16HandleException\num\()Ev:
	movb $\num, (interruptnumber)
	jmp int_bottom
	
.endm
# End Macro 2

# Macro 3: HandleInterruptRequest
.macro HandleInterruptRequest num
.global _ZN5rexos12hardwarecomm16InterruptManager26HandleInterruptRequest\num\()Ev

_ZN5rexos12hardwarecomm16InterruptManager26HandleInterruptRequest\num\()Ev:
	# moving number arg to interruptnumber (defined at the bottom)
	# IRQ_BASE is defined at the top
	movb $\num + IRQ_BASE, (interruptnumber)
	pushl $0; # for error value bc unless it's a real error, cpu doesnt
	# push anything and this is so the struct in multitasking works
	jmp int_bottom

.endm
# End Macro 3

# (I.5.)
# The following are copies of the above macros with different numbers for
# different interrupts. In order to use them as HandleInterruptRequest
# functions, we must define them down here.

# Timer
HandleInterruptRequest 0x00
# Keyboard
HandleInterruptRequest 0x01
# Mouse
HandleInterruptRequest 0x0C
# AM79C973
HandleInterruptRequest 0x09
# ATA Primary
HandleInterruptRequest 0x0E
# ATA Secondary
HandleInterruptRequest 0x0F
# Syscall (software) interrupt
HandleInterruptRequest 0x80

# (I.4.)
int_bottom:		# jump target for the macros 2 and 3
	# the contents of the registers might be important after returning from the
	# event handle. so we save them before jumping to the cpp function
	# pusha		# push all the registers
	# pushl %ds	# push the data segment
	# pushl %es
	# pushl %fs
	# pushl %gs
	
	pushl %ebp
	pushl %edi
	pushl %esi

	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax

	# call cpp handler
	# passing these to the cpp file's function and jumping to it
	pushl %esp
	push (interruptnumber)
	call _ZN5rexos12hardwarecomm16InterruptManager15handleInterruptEhj
	# add %esp, 6
	movl %eax, %esp # switch stack
	
	# restore the registers
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx

	popl %esi
	popl %edi
	popl %ebp
	# popl %gs
	# popl %fs
	# popl %es
	# popl %ds
	# popa

	add $4, %esp

# Macro 1: IgnoreInterruptRequest
.global _ZN5rexos12hardwarecomm16InterruptManager22IgnoreInterruptRequestEv
_ZN5rexos12hardwarecomm16InterruptManager22IgnoreInterruptRequestEv:
	iret	# exiting/returning from this code to the previous process's stack
# (I.2.)
.data
	interruptnumber: .byte 0	# interruptnumber is initilized as 0
