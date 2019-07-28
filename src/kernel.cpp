#include <common/types.h>
#include <gdt.h>
#include <hardwarecomm/interrupts.h>
#include <hardwarecomm/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

using namespace rexos;
using namespace rexos::common;
using namespace rexos::drivers;
using namespace rexos::hardwarecomm;
using namespace rexos::gui;

void printf(char* str) {
	/* There is a specific memory location = 0xb8000. Whatever is put there
	 * will be shown on the screen by the graphics card. 
	 * 		 	 ____ ____ ___ ____ ____ ___ (2 bytes: 1st for color
	 *		0xb8000:|0xFF|0x00| a |0xFF|0x00| b |	   2nd for character, MSB 0)
	 *  	    high (4) bits ^ 	^ low (4) bits (color information)
	 *   	     (1st) high byte ^ (default val: 0xFF00)
	 */
	uint16_t* VideoMemory = (uint16_t*) 0xb8000;
	// Cursor, so printf() call doesn't write to the same memory every time
	static uint8_t x = 0, y = 0;

	// Copy the passed string to this location
	for(int i=0; str[i] !='\0'; i++) {
		switch(str[i]) {
			// newline character moves the cursor down horizontally
			case '\n':
				y++;
				x = 0;
				break;
			// bitwise AND with 0xFF00 (black and white) will preserve
			// the color info of first byte. bitwise OR will append the
			// string after that.
		
			// Screen dimension is 80x25
			default:
				VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
				x++;
				break;
		}
		if(x >= 80) { // if x is greater than width move down
			y++;
			x = 0;
		}
		// Clear the screen when we reach the bottom
		if(y >= 25) {
			for(y = 0; y < 25; y++)
				for(x = 0; x < 80; x++)
					VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
			x = 0;
			y = 0;
		}
	}
}

void printfHex(uint8_t key) {
	char* foo = "00 ";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
// PrintfKeyboardHandler is a derived class of KeyboardEventHandler, which is later passed 
// to the keyboard driver function
// OnKeyDown is a virtual func in the base class KeyboardEventHandler
// so, OnKeyDown function can be overridden in this derived class

class PrintfKeyboardEventHandler : public KeyboardEventHandler {
	public:
		void OnKeyDown(char c) {
			char* foo = " ";
			foo[0] = c;
			printf(foo);
		}
		
};

class MouseToConsole : public MouseEventHandler {
	int8_t x, y;
	public:
		MouseToConsole() {
		// initializing cursor pixels at the center
		uint16_t* VideoMemory = (uint16_t*) 0xb8000;
		x = 40;
		y = 12;
		VideoMemory[80*12+40] = (VideoMemory[80*y+x] & 0xF000) >> 4 
							| (VideoMemory[80*y+x] & 0x0F00) << 4 // now shift the low 4 bits to high
							| (VideoMemory[80*y+x] & 0x00FF); // last 8 bits stay the same


		}
		
		
		
		
		void OnMouseMove(int x_offset, int y_offset) {
			uint16_t* VideoMemory = (uint16_t*) 0xb8000;
			VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xF000) >> 4 
					| (VideoMemory[80*y+x] & 0x0F00) << 4 // now shift the low 4 bits to high
					| (VideoMemory[80*y+x] & 0x00FF); // last 8 bits stay the same


			x += x_offset;

			if(x<0) x = 0;
			if(x>=80) x = 79;

			y += y_offset;
			if(y<0) y = 0;
			if(y>=25) y = 24;

			// after we moved the cursor
			VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xF000) /* take the high 4 bits */ 
									>> 4 /* shift 4 bits right so they're the new low bits */
					| (VideoMemory[80*y+x] & 0x0F00) << 4 // now shift the low 4 bits to high
					| (VideoMemory[80*y+x] & 0x00FF); // last 8 bits stay the same
		}

};

void taskA() {
	while(true)
		printf("A");
}
void taskB() {
	while(true)
		printf("B");
}




// Defining constructor. This is a function pointer
typedef void (*constructor)();
// extern "C" tells g++ compiler not to use its naming conventions in the .o 
// file so the linker.ld can work properly
extern "C" constructor start_ctors;	// addr of the first constructor call
extern "C" constructor end_ctors;	// after the addr of last constructor call
// Now we can use start_ctors and end_ctors as pointers to the constructor

// Iterates over the space between start_ctors and end_ctors and jumps into all
// the function pointers
extern "C" void callConstructors() {
	for(constructor* i = &start_ctors; i != &end_ctors; i++) {
			(*i)(); // Invoke (call) the constructors
	}
}

// kernel main function where all the necessary components are instantiated
extern "C" void kernelMain(void* multiboot_structure, 
		uint32_t magicnumber /*multiboot magic number*/) {
	/* (K.3.) Creating the GDT to set up a table with fields: base/ptr, size/
	limits, flags/access rights that can be used to keep records of the 
	code/data segments
	*/
	GlobalDescriptorTable gdt;
	// IntHandler needs to comm TM to do the scheduling
	TaskManager taskmngr;
	Task task1(&gdt, taskA);
	Task task2(&gdt, taskB);
	taskmngr.AddTask(&task1);
	taskmngr.AddTask(&task2);

	/* (K.4.) Passing the GDT's address to the interrupt manager to map the
	interrupts to the code segments defined by the GDT. In a userspace program
	when the CPU gets an interrupt, it has to move to kernel space where the
	interrupt handler is located. Interrupt manager's job is to switch the
	segments, access rights (0 - kernel space), and go to the interrupt handler
	*/
	rexos::hardwarecomm::InterruptManager interrupts(&gdt, &taskmngr);
	
	printf("Initializing Hardware, Stage 1\n");
	DriverManager drvManager;
	#ifdef GRAPHICSMODE
	Desktop desktop(320,200,0x00, 0x00, 0xA8);
	#endif
		
		#ifdef GRAPHICSMODE
		KeyboardDriver keyboard(&interrupts, &desktop);
		#endif
		PrintfKeyboardEventHandler kbhandler;
		KeyboardDriver keyboard(&interrupts, &kbhandler);
		drvManager.AddDriver(&keyboard);
		
		
		#ifdef GRAPHICSMODE
		MouseDriver mouse(&interrupts, &desktop);
		#endif
		MouseToConsole mousehandler;
		MouseDriver mouse(&interrupts, &mousehandler);
		drvManager.AddDriver(&mouse);

		PCIController PCICon;
		PCICon.SelectDriver(&drvManager, &interrupts);
		#ifdef GRAPHICSMODE
		VideoGraphicsArray vga;
		#endif
	
	printf("Initializing Hardware, Stage 2\n");
		drvManager.ActivateAll();

	printf("Initializing Hardware, Stage 3\n");
	
	// 320 is outside the range of uint8
	#ifdef GRAPHICSMODE
	vga.SetMode(320,200,8);
	Window win1(&desktop, 10,10,20,20, 0xA8,0x00,0x00);
	desktop.AddChild(&win1);
	Window win2(&desktop, 40,15,30,30, 0x00,0xA8,0x00);
	desktop.AddChild(&win2);
	#endif
	
	interrupts.Activate();
	
	
	while(1);
	#ifdef GRAPHICSMODE
		desktop.Draw(&vga);
	#endif
}
