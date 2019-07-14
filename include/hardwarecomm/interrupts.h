#ifndef __REXOS__HARDWARECOMM__INTERRUPTMANAGER_H
#define __REXOS__HARDWARECOMM__INTERRUPTMANAGER_H
#include <common/types.h>
#include <hardwarecomm/port.h>
#include <gdt.h>
namespace rexos {
	namespace hardwarecomm {
		class InterruptManager;
		// (I.24.) Purpose: Interface for HW drivers which the IM can use
		class InterruptHandler {
			protected:
				// This will know its own interrupt number
				rexos::common::uint8_t interruptNumber;
				// Ptr to the interrupt manager it's connected to
				InterruptManager* interruptManager;
				// Protected Ctor/Dtor so we cannot instantiate this
				// We dont have purely virtual functions so we dont have
				// abstract classes
				InterruptHandler(rexos::common::uint8_t interruptNumber, InterruptManager* interruptManager);
				~InterruptHandler();
			public:
				virtual rexos::common::uint32_t HandleInterrupt(rexos::common::uint32_t esp);
		};

	// (I.6.) Constructing the Interrupt Descriptor Table
		class InterruptManager {
			// (I.25.) Make InterruptManager and InterruptHandler class'
			// protected attributes available to each other
			friend class InterruptHandler;
			protected:
				// (I.20.) Purpose: To have access to the ports in order to
				// signal the PIC
				static InterruptManager* ActiveInterruptManager;
				// (I.26.) Something like an IDT but on a higher level
				// Hence, the InterruptHandler array of 256 entries
				InterruptHandler* handlers[256];
				// (I.7.) The entries of IDT are called Gate Descriptors
				struct GateDescriptor {
					// Ptr to the handler(split between low and high bits)
					rexos::common::uint16_t handlerAddressLowBits;
					// Offset of the code segment in the GDT
					rexos::common::uint16_t gdt_codeSegmentSelector;
					rexos::common::uint8_t reserved;	// 1 reserved byte
					rexos::common::uint8_t access;		// Access rights
					rexos::common::uint16_t handlerAddressHighBits;		
				} __attribute__((packed));	// Needs to be byte perfect
				// An array of 256 Gate Descriptor entries called IDT
				static GateDescriptor interruptDescriptorTable[256];
				
				// Purpose of the following is to tell the CPU to use this IDT
				struct InterruptDescriptorTablePointer {
					rexos::common::uint16_t size;	// limit/size
					rexos::common::uint32_t base;	// base/addr of the table
				} __attribute__((packed));
				
				// (I.8.) A function that sets entries in the IDT
				static void SetInterruptDescriptorTableEntry(
					// number of the interrupt
					rexos::common::uint8_t interruptNumber,
					// offset to the code segment
					rexos::common::uint16_t codeSegmentSelectorOffset,	
					// ptr to the handler function for given interrupt number
					void(*handler)(),
					rexos::common::uint8_t DescriptorPrivilegeLevel,  // access
					rexos::common::uint8_t DescriptorType);			  // flags

				// Finally, the purpose of the following is to tell the ports 
				// to give us the interrupts
				Port8BitSlow picMasterCommand;
				Port8BitSlow picMasterData;
				Port8BitSlow picSlaveCommand;
				Port8BitSlow picSlaveData;

			public:
			// Ctor for IM (gets a ptr to the GDT so gdt.h is included)
				InterruptManager(rexos::GlobalDescriptorTable* gdt);
				~InterruptManager();	// Dtor
				// Signal CPU to start interrupts
				void Activate();
				// Signal CPU to close interrupts		
				void Deactivate();
				static rexos::common::uint32_t handleInterrupt(rexos::common::uint8_t interruptNumber, rexos::common::uint32_t esp);
				// (I.21.) Static function handleInterrupt will call this non
				// static function of the object ActiveInterruptManager
				rexos::common::uint32_t DoHandleInterrupt(rexos::common::uint8_t interruptNumber, rexos::common::uint32_t esp);
				// (I.9.) the following functions are implemented/defined in the 
				// Macro 3 of interruptstubs.s
				static void IgnoreInterruptRequest();
				static void HandleInterruptRequest0x00();		// timer interrupt
				static void HandleInterruptRequest0x01();		// keyboard
				static void HandleInterruptRequest0x0C();		// mouse
		};
	}
}
#endif
