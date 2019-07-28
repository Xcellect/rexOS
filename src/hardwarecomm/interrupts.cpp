#include <hardwarecomm/interrupts.h>
using namespace rexos;
using namespace rexos::common;
using namespace rexos::hardwarecomm;

void printf(char* str);
void printfHex(uint8_t);

// (I.27.) Following is a constructor for InterruptHandler
// eg. for kb the number is 0x21
InterruptHandler::InterruptHandler(uint8_t interruptNumber, 
	InterruptManager* interruptManager) {
		this->interruptNumber = interruptNumber;
		this->interruptManager = interruptManager;
		// Putting itself in the array of handlers
		interruptManager->handlers[interruptNumber] = this;
}
InterruptHandler::~InterruptHandler() {
	if(interruptManager->handlers[interruptNumber] == this) {
		interruptManager->handlers[interruptNumber] = 0;
	}
}
	
uint32_t InterruptHandler::HandleInterrupt(uint32_t esp) {
	return esp;
}

// (I.11.) Declaring the IDT with 256 entries
InterruptManager::GateDescriptor InterruptManager::interruptDescriptorTable[256];
// (I.20.) Initializing Active Interrupt Manager as 0
InterruptManager* InterruptManager::ActiveInterruptManager = 0;

// (I.12.) A function to set up the IDT
void InterruptManager::SetInterruptDescriptorTableEntry(
	uint8_t interruptNumber,
	uint16_t codeSegmentSelectorOffset,
	void(*handler)(),
	uint8_t DescriptorPrivilegeLevel,
	uint8_t DescriptorType) {
	
	// Current IDT = 128 or 0x80
	const uint8_t IDT_DESC_PRESENT = 0x80;

	// take the low bits of the address
	interruptDescriptorTable[interruptNumber].handlerAddressLowBits = ((uint32_t)handler) & 0xFFFF;
	// take the high bits of the address
	interruptDescriptorTable[interruptNumber].handlerAddressHighBits = (((uint32_t)handler) >> 16) & 0xFFFF;
	// use the passed value for offset
	interruptDescriptorTable[interruptNumber].gdt_codeSegmentSelector = codeSegmentSelectorOffset;
	// access rights
	interruptDescriptorTable[interruptNumber].access = IDT_DESC_PRESENT | DescriptorType | ((DescriptorPrivilegeLevel & 3) << 5);
	interruptDescriptorTable[interruptNumber].reserved = 0;
}	

// First a new gdt is initilized and passed to this funtion (in the kernel)
// then address of this function is sent to the KeyboardDriver
// KeyboardDriver accepts is as a pointer (since address points to some data)
// in the contructor initilization list, it's also passed to the InterruptHandler 
// at interrupts.cpp::6

// (I.13.) InterruptManager constructor uses GDT to map interrupt handlers
InterruptManager::InterruptManager(GlobalDescriptorTable* gdt, TaskManager* taskManager)
: picMasterCommand(0x20),	// (I.14.) Instantiating the ports
  picMasterData(0x21),
  picSlaveCommand(0xA0),
  picSlaveData(0xA1)
{
	this->taskManager = taskManager;
	// (I.15.) Set the code seg, interrupt gate to 14 (0xE) and initialize all
	// to ignore
	uint16_t codeSegmentOffset = gdt->CodeSegmentOffset();
	// Type of the interrupt gate = 14 or 0xE
	const uint8_t IDT_INTERRUPT_GATE = 0xE;
	// Initialize all the entries to ignore interrupt
	for(uint16_t i = 0; i < 256; i++) {
		// (I.28.) No handlers yet 
		handlers[i] = 0;
		// For the ith entry we take the code seg offset from the GDT,
		// The address of ignore function (<- mapping happens here) 
		// Privilege level 0 (kernel space),
		// The type of interrupt gate
		SetInterruptDescriptorTableEntry(i, codeSegmentOffset, &IgnoreInterruptRequest, 0, IDT_INTERRUPT_GATE);
	}
	handlers[0] = 0;
	SetInterruptDescriptorTableEntry(0, codeSegmentOffset, &IgnoreInterruptRequest, 0, IDT_INTERRUPT_GATE);

	// IRQ_BASE (interruptstubs.s) is set to 0x20 so handle interrupt number,
	// 0x00, 0x01, 0x0C etc are added to that before passing

	// (I.16.) We want to handle the following interrupts
	SetInterruptDescriptorTableEntry(0x20, codeSegmentOffset, &HandleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x21, codeSegmentOffset, &HandleInterruptRequest0x01, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x2C, codeSegmentOffset, &HandleInterruptRequest0x0C, 0, IDT_INTERRUPT_GATE);

	/* Example: If we get an interrupt 0x21 or 33, we'll jump into  
	HandleInterruptRequest0x01 in interruptstubs.s, which is a copy of macro
	"HandleInterruptRequest num" where the IRQ_BASE (0x20) is added to 0x01 and
	we'll get a 0x21 which is pushed to the stack and handleInterrupt function
	is called.
	*/

	// (I.17.) Before signaling the CPU to use this table, we communicate with these
	// PICs (ports).
	picMasterCommand.Write(0x11);
	picSlaveCommand.Write(0x11);
	// hardware interrupt offset is 0x20
	picMasterData.Write(0x20);
	picSlaveData.Write(0x28);	
	// You get int 1 when you press a key on the keyboard. int 1 is also
	// used for internal exceptions by the CPU. so master pic is told to 
	// add 0x20 to it, slave pic is told to add 0x28 to it. bc both of these
	// pics have 8 ints - master (0x20 - 0x27), slave (0x28 to 0x30)
	picMasterData.Write(0x04); //initializes PIC as master
	picSlaveData.Write(0x02); //initializes PIC as slave

	picMasterData.Write(0x01);
	picSlaveData.Write(0x01);
	
	picMasterData.Write(0x00);
	picSlaveData.Write(0x00);
	
	// (I.18.) Tell the processor to use the IDT
	InterruptDescriptorTablePointer idt;
	idt.size = 256 * sizeof(GateDescriptor) - 1;	// We have 256 entries
	idt.base = (uint32_t) interruptDescriptorTable;	// Ptr to this IDT
	asm volatile("lidt %0" :: "m" (idt));			// Load this table
	}
InterruptManager::~InterruptManager() {
	Deactivate();
}

/* (I.19.) Start/closing interrupts are in separate functions because in the kernel
the GDT, Interrupt Manager (&gdt), different hardwares(&IM, &HWSpecificHandler)
need to be declared successively before the CPU can start interrupts.
Else, there wouldn't be any hardware to receive interrupts from.
 */

void InterruptManager::Activate() {
	// (I.22.) If an IM is set already, deactivate it first
	if(ActiveInterruptManager != 0)
		ActiveInterruptManager->Deactivate();
	// Set this Active IM ptr to this instance 
	ActiveInterruptManager = this;
	asm("sti");
}


void InterruptManager::Deactivate() {
	if(ActiveInterruptManager == this) {
		ActiveInterruptManager = 0;
		asm("cli");
	}
}

uint32_t InterruptManager::handleInterrupt(uint8_t interruptNumber, uint32_t esp) {
	// (I.23.)
	if(ActiveInterruptManager != 0)
		return ActiveInterruptManager->DoHandleInterrupt(interruptNumber, esp);
	return esp;
}
// Same thing as handleInterrupt but on an object (non static)
// Here we have access to the port functions
uint32_t InterruptManager::DoHandleInterrupt(uint8_t interruptNumber, uint32_t esp) {
	// (I.29.) If we have a handler for it, call the HandleInterrupt function
	if(handlers[interruptNumber] != 0) {
		esp = handlers[interruptNumber]->HandleInterrupt(esp);
	} else if(interruptNumber != 0x20) {
		// Tells the type of unhandled interrupt
		printf("UNHANDLED INTERRUPT 0x");
		printfHex(interruptNumber);
	}
	// If we have a timer interrupt
	if(interruptNumber == 0x20) {
		esp = (uint32_t)taskManager->Schedule((CPUState*)esp);
	}


	// Time to reply to the HW interrupts
	// We have mapped hardwares from 0x20 to 0x30
	if(0x20 <= interruptNumber && interruptNumber < 0x30) {
		// Answer to the master PIC
		picMasterCommand.Write(0x20);
		if(0x28 <= interruptNumber) {
			// Answer slave only if it's >= 0x28
			picSlaveCommand.Write(0x20);  // !!!!! didn't work bc of Write's arg not being 0x20		
		}
	}

	return esp;
}
