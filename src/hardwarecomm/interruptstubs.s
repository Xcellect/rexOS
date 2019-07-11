.set IRQ_BASE, 0x20	# IRQ_BASE (compiler variable) is set to 32 or 0x20
.section .text
# These markers/landing points won't work after changing namespace
# constructed as following:
# 1. starts with _ZN
# 2. then class/namespace/method names (eg. InterruptManager)
# 3. length of each name precedes it
# 4. finally the parameters for the method (eg. Ehj, Ev)


# Following is an external function from a different object (cpp) file. Declared 
# like a function declaration on C/C++ (eg. the printf function defined in  
# kernel is declared whenever a different code file needed to use it)
.extern _ZN5rexos12hardwarecomm16InterruptManager15handleInterruptEhj

# Internal function 1: IgnoreInterruptRequest
.global _ZN5rexos12hardwarecomm16InterruptManager22IgnoreInterruptRequestEv

# Start internal function 2: HandleException
.macro HandleException num
.global _ZN5rexos12hardwarecomm16InterruptManager16HandleException\num\()Ev

_ZN5rexos12hardwarecomm16InterruptManager16HandleException\num\()Ev:
	movb $\num, (interruptnumber)
	jmp int_bottom
	
.endm
# End internal function 2

# Interrupt function 3: HandleInterruptRequest
.macro HandleInterruptRequest num
.global _ZN5rexos12hardwarecomm16InterruptManager26HandleInterruptRequest\num\()Ev

_ZN5rexos12hardwarecomm16InterruptManager26HandleInterruptRequest\num\()Ev:
	# moving number arg to interruptnumber (defined at the bottom)
	# IRQ_BASE is defined at the top
	movb $\num + IRQ_BASE, (interruptnumber)
	jmp int_bottom

.endm
# End internal function 3

# The following are copies of the above macros with different numbers for
# different interrupts. In order to use them as HandleInterruptRequest
# functions, we must define them down here.

# Timer
HandleInterruptRequest 0x00
# Keyboard
HandleInterruptRequest 0x01
# Mouse
HandleInterruptRequest 0x0C

int_bottom:		# jump target for the functions 2 and 3
	# the contents of the registers might be important after returning from the
	# event handle. so we save them before jumping to the cpp function
	pusha		# push all the registers
	pushl %ds	# push the data segment
	pushl %es
	pushl %fs
	pushl %gs
	
	# passing these to the cpp file's function and jumping to it
	pushl %esp
	push (interruptnumber)
	call _ZN5rexos12hardwarecomm16InterruptManager15handleInterruptEhj
	add %esp, 6
	movl %eax, %esp
	
	# restore the values
	popl %gs
	popl %fs
	popl %es
	popl %ds
	popa

_ZN5rexos12hardwarecomm16InterruptManager22IgnoreInterruptRequestEv:
	iret	# exiting/returning from this code to the previous process's stack

.data
	interruptnumber: .byte 0	# interruptnumber is initilized as 0
