# the second param tells the compiler to look for the headers in the include directory
# reason why include is done with corner braces
# definition of flags:
# -fno-use-cxa-atexit: no memory management 
# -nostdlib: no glibc library
# -fno-rtti: no runtime type identification
# -fno-leading-underscore (minor): without this the loader would have to call _kernelMain
GPPPARAMS = -m32 -Iinclude -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
ASPARAMS = --32
LDPARAMS = -melf_i386

objects = obj/loader.o \
		obj/gdt.o \
		obj/heap.o \
		obj/drivers/driver.o \
		obj/hardwarecomm/port.o \
		obj/hardwarecomm/interruptstubs.o \
		obj/hardwarecomm/interrupts.o \
		obj/drivers/syscalls.o \
		obj/multitasking.o \
		obj/drivers/amd_am79c973.o \
		obj/hardwarecomm/pci.o \
		obj/drivers/keyboard.o \
		obj/drivers/mouse.o \
		obj/drivers/vga.o \
		obj/drivers/ata.o \
		obj/gui/widget.o \
		obj/gui/window.o \
		obj/gui/desktop.o \
		obj/net/ethframe.o \
		obj/kernel.o

obj/%.o: src/%.cpp
	mkdir -p $(@D)
	g++ $(GPPPARAMS) -o $@ -c $<

obj/%.o: src/%.s
	mkdir -p $(@D)
	as $(ASPARAMS) -o $@ $<

rexKernel.bin: linker.ld $(objects)
	ld $(LDPARAMS) -T $< -o $@ $(objects)

install: rexKernel.bin
	sudo cp $< /boot/rexKernel.bin
rexKernel.iso: rexKernel.bin
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp $< iso/boot/
	echo 'set timeout=0' >> iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	# The menuentry text is showing up in the heap
	echo 'menuentry "Rex OS" {' >> iso/boot/grub/grub.cfg
	echo '	multiboot /boot/rexKernel.bin' >> iso/boot/grub/grub.cfg
	echo '	boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=$@ iso
	rm -rf iso
run: rexKernel.iso
	(killall VirtualBox && sleep 1) || true
	VBoxManage startvm "RexOS" &
.PHONY: clean
clean:
	rm -rf obj rexKernel.bin rexKernel.iso
