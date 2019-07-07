#include "types.h"
#include "gdt.h"
#include "interrupts.h"
#include "keyboard.h"
#include "mouse.h"

void printf(char* str) {
	/* There is a specific memory location = 0xb8000. Whatever is put there
	 * will be shown on the screen by the graphics card. 
	 * 		 	 	 ____ ____ ___ _____ ___      (2 bytes: 1st for color
	 *		0xb8000:|0xFF|0x00| a |  |  | b |	   			2nd for character)
	 *   high (4) bits ^ 	^ low (4) bits (color information)
	 *   (1st) high byte ^ (default val: 0xFF00)
	 */
	uint16_t* VideoMemory = (uint16_t*) 0xb8000;
	// Cursor
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
		if(x >= 80) {
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
// Defining constructor. This is a function pointer
typedef void (*constructor)();
// extern "C" tells g++ compiler not to use its naming conventions in the .o 
// file so the loader.s can work properly
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

extern "C" void kernelMain(void* multiboot_structure, 
		uint32_t magicnumber /*multiboot magic number*/) {
	printf("Head bend over\n");
	printf("Raise that posterior ");
	GlobalDescriptorTable gdt;
	InterruptManager interrupts(&gdt);
	KeyboardDriver keyboard(&interrupts);
	MouseDriver mouse(&interrupts);


	interrupts.Activate();
	while(1);
}
