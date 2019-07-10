#include <hardwarecomm/interrupts.h>
using namespace rexos::common;
using namespace rexos::hardwarecomm;

void printf(char* str);
void printfHex(uint8_t);

// InterruptManager func ptr is caught here to initialize the InterruptHandler
// following is a constructor for InterruptHandler
// eg. for kb the number is 0x21
InterruptHandler::InterruptHandler(uint8_t interruptNumber, 
	InterruptManager* interruptManager) {
		this->interruptNumber = interruptNumber;
		this->interruptManager = interruptManager;
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



InterruptManager::GateDescriptor InterruptManager::interruptDescriptorTable[256];
InterruptManager* InterruptManager::ActiveInterruptManager = 0;
void InterruptManager::SetInterruptDescriptorTableEntry(
	uint8_t interruptNumber,
	uint16_t codeSegmentSelectorOffset,
	void(*handler)(),
	uint8_t DescriptorPrivilegeLevel,
	uint8_t DescriptorType) {

	const uint8_t IDT_DESC_PRESENT = 0x80;

	interruptDescriptorTable[interruptNumber].handlerAddressLowBits = ((uint32_t)handler) & 0xFFFF;
	interruptDescriptorTable[interruptNumber].handlerAddressHighBits = (((uint32_t)handler) >> 16) & 0xFFFF;
	interruptDescriptorTable[interruptNumber].gdt_codeSegmentSelector = codeSegmentSelectorOffset;
	interruptDescriptorTable[interruptNumber].access = IDT_DESC_PRESENT | DescriptorType | ((DescriptorPrivilegeLevel & 3) << 5);
	interruptDescriptorTable[interruptNumber].reserved = 0;
}	
// first a new gdt is initilized and passed to this funtion (in the kernel)
// then address of this function is sent to the KeyboardDriver
// KeyboardDriver accepts is as a pointer (since address points to some data)
// in the contructor initilization list, it's also passed to the InterruptHandler at interrupts.cpp::6
InterruptManager::InterruptManager(GlobalDescriptorTable* gdt)
: picMasterCommand(0x20),
  picMasterData(0x21),
  picSlaveCommand(0xA0),
  picSlaveData(0xA1)
{
	uint16_t codeSegmentOffset = gdt->CodeSegmentOffset();
	const uint8_t IDT_INTERRUPT_GATE = 0xE;
	for(uint16_t i = 0; i < 256; i++) {
		handlers[i] = 0;
		SetInterruptDescriptorTableEntry(i, codeSegmentOffset, &IgnoreInterruptRequest, 0, IDT_INTERRUPT_GATE);
	}
	handlers[0] = 0;
	SetInterruptDescriptorTableEntry(0, codeSegmentOffset, &IgnoreInterruptRequest, 0, IDT_INTERRUPT_GATE);

	SetInterruptDescriptorTableEntry(0x20, codeSegmentOffset, &HandleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x21, codeSegmentOffset, &HandleInterruptRequest0x01, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x2C, codeSegmentOffset, &HandleInterruptRequest0x0C, 0, IDT_INTERRUPT_GATE);

	
	picMasterCommand.Write(0x11);
	picSlaveCommand.Write(0x11);
	// hardware interrupt offset is 0x20
	picMasterData.Write(0x20);
	picSlaveData.Write(0x28);	
	// you get interrupt 1 when you press a key on the keyboard. interrupt 1 is also
	// used for internal exceptions. so master pic is told to add 0x20 to it, slave pic
	// is told to add 0x28 to it. bc both of these pics have 8 interrupts
	picMasterData.Write(0x04); //initializes as master
	picSlaveData.Write(0x02); //initializes as slave

	picMasterData.Write(0x01);
	picSlaveData.Write(0x01);
	
	picMasterData.Write(0x00);
	picSlaveData.Write(0x00);
	
	InterruptDescriptorTablePointer idt;
	idt.size = 256 * sizeof(GateDescriptor) - 1;
	idt.base = (uint32_t) interruptDescriptorTable;
	asm volatile("lidt %0" :: "m" (idt));
	}
InterruptManager::~InterruptManager() {
	Deactivate();
}

void InterruptManager::Activate() {
	if(ActiveInterruptManager != 0)
		ActiveInterruptManager->Deactivate();
	ActiveInterruptManager = this;
	asm("sti");
}


void InterruptManager::Deactivate() {
	if(ActiveInterruptManager != this) {
		ActiveInterruptManager = 0;
		asm("cli");
	}
}

uint32_t InterruptManager::handleInterrupt(uint8_t interruptNumber, uint32_t esp) {
	if(ActiveInterruptManager != 0)
		return ActiveInterruptManager->DoHandleInterrupt(interruptNumber, esp);
	return esp;
}

uint32_t InterruptManager::DoHandleInterrupt(uint8_t interruptNumber, uint32_t esp) {
	if(handlers[interruptNumber] != 0) {
		esp = handlers[interruptNumber]->HandleInterrupt(esp);
	} else if(interruptNumber != 0x20) {
		// Tells the type of unhandled interrupt
		printf("UNHANDLED INTERRUPT 0x");
		printfHex(interruptNumber);
	}
	// Time to get HW interrupts
	// We have mapped hardwares from 0x20 to 0x30
	if(0x20 <= interruptNumber && interruptNumber < 0x30) {
		picMasterCommand.Write(0x20);
		if(0x28 <= interruptNumber) {
			picSlaveCommand.Write(0x20);	// !!!!! didn't work bc of Write arg not being 0x20		
		}
	}

	return esp;
}
