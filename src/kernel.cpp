#include <common/types.h>
#include <gdt.h>
#include <heap.h>
#include <hardwarecomm/interrupts.h>
#include <drivers/syscalls.h>
#include <hardwarecomm/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

#include <drivers/amd_am79c973.h>
#include <net/ethframe.h>
#include <net/arp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>

using namespace rexos;
using namespace rexos::common;
using namespace rexos::drivers;
using namespace rexos::hardwarecomm;
using namespace rexos::gui;
using namespace rexos::net;
// #define GRAPHICSMODE

void printf(char* str) {
	/* There is a specific memory location = 0xb8000. Whatever is put there
	 * will be shown on the screen by the graphics card. 
	 * 		 	 ____ ____ ___ ____ ____ ___ (2 bytes: 1st for color
	 *		0xb8000:|0xFF|0x00| a |0xFF|0x00| b |	   2nd for character, MSB 0)
	 *  	    high (4) bits ^ 	^ low (4) bits (color information)
	 *   	     (1st) high byte ^ (default val: 0xFF00)
	 */
	static uint16_t* VideoMemory = (uint16_t*) 0xb8000;
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
	char* foo = "00";
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
// When we get data, we'll print out every character one at a time
// One could set data + size to \0 and print the whole data but that
// would possibly remove the last character and if the next character
// was set to \n, it would break the memory manager
class PrintfUDPHandler : public UDPHandler {
	public:
		void HandleUDPMessage(UDPSocket* socket, uint8_t* data, uint16_t size) {
			// printf("\nRECEIVING [Layer 4]: ");
			char* foo = " ";
			// printf("\nRAW: ");
			for(int i = 0; i < (size > 64 ? 64 : size); i++) {
				printfHex(data[i]);
				printf(" ");
			}
			// printf("\nASCII: ");
			for(int i = 0; i < (size > 16 ? 16 : size); i++) {
				foo[0] = data[i];
				if(foo[0] == '\0') {
					break;
				}
				printf(foo);
			}
		}
};

class PrintfTCPHandler : public TCPHandler {
	public:
		bool HandleTCPMessage(TCPSocket* socket, uint8_t* data, uint16_t size) {
			printf("\nRCV [TCP]: ");
			char* foo = " ";
			// printf("\nRAW: ");
			for(int i = 0; i < (size > 64 ? 64 : size); i++) {
				printfHex(data[i]);
				printf(" ");
			}
			// printf("\nASCII: ");
			for(int i = 0; i < (size > 16 ? 16 : size); i++) {
				foo[0] = data[i];
				if(foo[0] == '\0') {
					break;
				}
				printf(foo);
			}
			if(size > 4 &&
				data[0] == 'G' &&
				data[1] == 'E' &&
				data[2] == 'T' &&
				data[3] == ' ' &&
				data[4] == '/' &&
				data[5] == ' ' &&
				data[6] == 'H' &&
				data[7] == 'T' &&
				data[8] == 'T' &&
				data[9] == 'P') {
					socket->Send((uint8_t*)"HTTP/1.1 200 OK\r\nServer: RexOS\r\nContent-Type: text/html\r\n\r\n<html><head><title> Sup RexOS </title></head><body><b>BLAZE IT REX</b><br> This is lit af </body> </html>\r\n", 167);
					socket->Disconnect();
					// Ungraceful: return false;
				}
			return true;
		}
};





void sysprintf(char* str) {
	asm("int $0x80" :: "a" (4), "b" (str));
}

// Now the tasks can call what's legal in usermode
void taskA() {
	while(true)
		sysprintf("A");
}
void taskB() {
	while(true)
		sysprintf("B");
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
	uint32_t* memupper = (uint32_t*)(((size_t)multiboot_structure) + 8);
	size_t heap = 10*1024*1024;
	//Instantiating dynamic memory management/heap
	MemoryManager mem_manage(heap, (*memupper)*1024 - heap - 10*1024);
	/*
	printf("\nHeap: 0x");
	printfHex((heap >> 24) & 0xFF);
	printfHex((heap >> 16) & 0xFF);
	printfHex((heap >> 8) & 0xFF);
	printfHex((heap) & 0xFF);

	void* allocated = mem_manage.malloc(1024);
	printf("\nAllocated: 0x");
	printfHex(((size_t)allocated >> 24) & 0xFF);
	printfHex(((size_t)allocated >> 16) & 0xFF);
	printfHex(((size_t)allocated >> 8) & 0xFF);
	printfHex(((size_t)allocated) & 0xFF);
	printf("\n");
	*/
	// IntHandler needs to comm TM to do the scheduling
	TaskManager taskmngr;
	/* 
	Task task1(&gdt, taskA);
	Task task2(&gdt, taskB);
	taskmngr.AddTask(&task1);
	taskmngr.AddTask(&task2);
	*/
	/* (K.4.) Passing the GDT's address to the interrupt manager to map the
	interrupts to the code segments defined by the GDT. In a userspace program
	when the CPU gets an interrupt, it has to move to kernel space where the
	interrupt handler is located. Interrupt manager's job is to switch the
	segments, access rights (0 - kernel space), and go to the interrupt handler
	*/
	rexos::hardwarecomm::InterruptManager interrupts(0x20, &gdt, &taskmngr);
	// Syscall is int 0x80
	SyscallHandler syscalls(&interrupts, 0x80);


	printf("Initializing Hardware, Stage 1\n");
	DriverManager drvManager;
	#ifdef GRAPHICSMODE
	Desktop desktop(320,200,0x00, 0x00, 0xA8);
	#endif
		
		#ifdef GRAPHICSMODE
		KeyboardDriver keyboard(&interrupts, &desktop);
		#else
		PrintfKeyboardEventHandler kbhandler;
		KeyboardDriver keyboard(&interrupts, &kbhandler);
		#endif
		drvManager.AddDriver(&keyboard);
		
		
		#ifdef GRAPHICSMODE
		MouseDriver mouse(&interrupts, &desktop);
		#else
		MouseToConsole mousehandler;
		MouseDriver mouse(&interrupts, &mousehandler);
		#endif
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
	/*
	// interrupt 14
	ATA ata0m(0x1F0, true);
	printf("ATA Primary Master: ");
	ata0m.Identify();
	printf("\n");

	ATA ata0s(0x1F0, false);
	printf("ATA Primary Slave: ");
	ata0s.Identify();
	printf("\n");


	char* ataBuffer = "TESTING";
	ata0s.Write28(0, (uint8_t*) ataBuffer, 7);
	ata0s.Flush();
	printf("\n");
	ata0s.Read28(0,7);

	// interrupt 15
	ATA ata1m(0x170, true);
	ATA ata1s(0x170, false);
	*/	

	

	// third: 0x1E8
	// fourth: 0x168

	uint8_t ip1 = 10, ip2 = 0, ip3 = 2, ip4 = 15;
	uint32_t ip_be = ((uint32_t)ip4) << 24
					| ((uint32_t)ip3) << 16
					| ((uint32_t)ip2) << 8
					| ((uint32_t)ip1);
	uint8_t gip1 = 10, gip2 = 0, gip3 = 2, gip4 = 2;	// gateway
	uint32_t gip_be = ((uint32_t)gip4) << 24
					| ((uint32_t)gip3) << 16
					| ((uint32_t)gip2) << 8
					| ((uint32_t)gip1);

	uint8_t subnet1 = 255, subnet2 = 255, subnet3 = 255, subnet4 = 0;	// gateway
	uint32_t subnet_be = ((uint32_t)subnet4) << 24
					| ((uint32_t)subnet3) << 16
					| ((uint32_t)subnet2) << 8
					| ((uint32_t)subnet1);

	
	// just for testing
	amd_am79c973* eth0 = (amd_am79c973*) (drvManager.drivers[2]);

	
	// Setting the IP at layer 2 so other layers can request it later
	eth0->SetIPAddress(ip_be);
	
	//eth0->Send((uint8_t*) "AAA", 3);
	// Attaching layer 2 (ethFrame) to layer 1 (driver)
	EthernetFrameProvider ethFrame(eth0);	
	// Attaching layer 3 (network/ARP) to layer 2 (data link/ethernet)
	AddressResolutionProtocol arp(&ethFrame);
	
	IPv4Provider ipv4(&ethFrame, &arp, gip_be,subnet_be);
	ICMP icmp(&ipv4);
	UDPProvider udp(&ipv4);
	TCPProvider tcp(&ipv4);
	//ethFrame.Send(0xFFFFFFFFFFFF, 0x0608, (uint8_t*) "FUCK THE WORLD", 14);
	
	// We can only get reply to the requests at layer 3 after the interrupts
	// are activated
	interrupts.Activate();
	// arp.Resolve(gip_be);
    // ipv4.Send(gip_be, 0x0008, (uint8_t*) "foobar", 6);
	arp.SendMACAddress(gip_be);
	
	// icmp.RequestEchoReply(gip_be);
	// PrintfUDPHandler udphandler;
	
	PrintfTCPHandler tcphandler;
	TCPSocket* tcpsock = tcp.Listen(31337);
	tcp.Bind(tcpsock, &tcphandler);
	// tcpsock->Send((uint8_t*)"AAAAAAAAA", 9);
	
	
	// UDPSocket* udpsocket = udp.Listen(1337);
	// udp.Bind(udpsocket, &udphandler);
	
	
	/*
	UDPSocket* udpsocket = udp.Connect(gip_be, 31337);
	udp.Bind(udpsocket, &udphandler);
	udpsocket->Send((uint8_t*)"AAAAAAAAA", 9);
	*/
	while(1) {
		#ifdef GRAPHICSMODE
		desktop.Draw(&vga);
		#endif
	}
}
