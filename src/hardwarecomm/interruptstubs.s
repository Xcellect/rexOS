.set IRQ_BASE, 0x20
.section .text
# these markers/landing points won't work after changing namespace
# constructed as following:
# 1. starts with _ZN
# 2. then class/namespace/method names (eg. InterruptManager)
# 3. length of each name precedes it
# 4. finally the parameters for the method (eg. Ehj, Ev)

.extern _ZN5rexos12hardwarecomm16InterruptManager15handleInterruptEhj
.global _ZN5rexos12hardwarecomm16InterruptManager22IgnoreInterruptRequestEv

.macro HandleException num
.global _ZN5rexos12hardwarecomm16InterruptManager16HandleException\num\()Ev

_ZN5rexos12hardwarecomm16InterruptManager16HandleException\num\()Ev:
	movb $\num, (interruptnumber)
	jmp int_bottom
	
.endm

# asm function for handling interrupt by the cpu
.macro HandleInterruptRequest num
.global _ZN5rexos12hardwarecomm16InterruptManager26HandleInterruptRequest\num\()Ev

_ZN5rexos12hardwarecomm16InterruptManager26HandleInterruptRequest\num\()Ev:
	movb $\num + IRQ_BASE, (interruptnumber)
	jmp int_bottom

.endm

# we copy this as many times we need interrupts
# keyboard/misc
HandleInterruptRequest 0x00
HandleInterruptRequest 0x01
# mouse
HandleInterruptRequest 0x0C

int_bottom:

	pusha
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs

	pushl %esp
	push (interruptnumber)
	call _ZN5rexos12hardwarecomm16InterruptManager15handleInterruptEhj
	add %esp, 6
	movl %eax, %esp
	
	popl %gs
	popl %fs
	popl %es
	popl %ds
	popa

_ZN5rexos12hardwarecomm16InterruptManager22IgnoreInterruptRequestEv:

	iret

.data
	interruptnumber: .byte 0
