#ifndef __REXOS__HARDWARECOMM__INTERRUPTMANAGER_H
#define __REXOS__HARDWARECOMM__INTERRUPTMANAGER_H
#include <common/types.h>
#include <hardwarecomm/port.h>
#include <gdt.h>
namespace rexos {
	namespace hardwarecomm {
		class InterruptManager;

		class InterruptHandler {
			protected:
				rexos::common::uint8_t interruptNumber;
				InterruptManager* interruptManager;

				InterruptHandler(rexos::common::uint8_t interruptNumber, InterruptManager* interruptManager);
				~InterruptHandler();
			public:
				virtual rexos::common::uint32_t HandleInterrupt(rexos::common::uint32_t esp);
		};


		class InterruptManager {
			friend class InterruptHandler;
			protected:
				static InterruptManager* ActiveInterruptManager;
				InterruptHandler* handlers[256];
				
				struct GateDescriptor {
					rexos::common::uint16_t handlerAddressLowBits;
					rexos::common::uint16_t gdt_codeSegmentSelector;
					rexos::common::uint8_t reserved;
					rexos::common::uint8_t access;
					rexos::common::uint16_t handlerAddressHighBits;		
				} __attribute__((packed));
				
				static GateDescriptor interruptDescriptorTable[256];
				
				struct InterruptDescriptorTablePointer {
					rexos::common::uint16_t size;
					rexos::common::uint32_t base;
				} __attribute__((packed));

				static void SetInterruptDescriptorTableEntry(
						rexos::common::uint8_t interruptNumber,
						rexos::common::uint16_t codeSegmentSelectorOffset,
						void(*handler)(),
						rexos::common::uint8_t DescriptorPrivilegeLevel,
						rexos::common::uint8_t DescriptorType);

				Port8BitSlow picMasterCommand;
				Port8BitSlow picMasterData;
				Port8BitSlow picSlaveCommand;
				Port8BitSlow picSlaveData;

			public:
				InterruptManager(rexos::GlobalDescriptorTable* gdt);
				~InterruptManager();
				void Activate();
				void Deactivate();
				static rexos::common::uint32_t handleInterrupt(rexos::common::uint8_t interruptNumber, rexos::common::uint32_t esp);
				rexos::common::uint32_t DoHandleInterrupt(rexos::common::uint8_t interruptNumber, rexos::common::uint32_t esp);
				static void IgnoreInterruptRequest();
				static void HandleInterruptRequest0x00();
				static void HandleInterruptRequest0x01();
				static void HandleInterruptRequest0x0C();
		};
	}
}
#endif
