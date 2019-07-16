#ifndef __REXOS__HARDWARECOMM__PCI_H
#define __REXOS__HARDWARECOMM__PCI_H
#include <hardwarecomm/port.h>
#include <drivers/driver.h>
#include <common/types.h>
#include <hardwarecomm/interrupts.h>   
namespace rexos {
    namespace hardwarecomm {
        // (P.16.) Purpose: Utilizing base address registers at offset 0xE of
        // PCI configuration of the device.
        // To distinguish between I/O or memory space/type BARs
        enum BaseAddressRegisterType {
            MemoryMapping = 0,
            InputOutput = 1
        };
        // (P.17.) BAR class
        class BaseAddressRegister {
            public:
                bool prefetchable;  // For memory mapping BARs
                rexos::common::uint8_t* address;
                rexos::common::uint8_t size;
                BaseAddressRegisterType type;
        };


        // (P.12.) Purpose of this class is to store the device identifiers
        class PCIDeviceDescriptor {
            public:
                rexos::common::uint32_t portBase;
                rexos::common::uint32_t interrupt;

                rexos::common::uint16_t bus;
                rexos::common::uint16_t device;
                rexos::common::uint16_t function;
                
                rexos::common::uint16_t vendor_id;
                rexos::common::uint16_t device_id;

                rexos::common::uint8_t class_id;
                rexos::common::uint8_t subclass_id;
                rexos::common::uint8_t interface_id;

                rexos::common::uint8_t revision;

            PCIDeviceDescriptor();
            ~PCIDeviceDescriptor();
        };


        class PCIController {
            // (P.1.) Standard data and command ports
            rexos::hardwarecomm::Port32Bit dataport;
            rexos::hardwarecomm::Port32Bit commandport;

            public:
                PCIController();
                ~PCIController();

            // (P.2.) Purpose: To read and write data from device on PCI controller.
            // This function takes bus, device, function numbers. We will be able
            // to read data from certain function number. These function numbers
            // have memory locations and standardized memory space. So we have to 
            // read certain offsets of that memory where we'll find vendor, device,
            // class, subclass etc id. We also have to pass the offsets of the data
            // (class, subclass etc). Extra: write value for the Write function.
            rexos::common::uint32_t Read(rexos::common::uint16_t bus, 
                rexos::common::uint16_t device, 
                rexos::common::uint16_t function,
                rexos::common::uint16_t registeroffset);
            void Write(rexos::common::uint16_t bus, 
                rexos::common::uint16_t device, 
                rexos::common::uint16_t function,
                rexos::common::uint32_t registeroffset,
                rexos::common::uint32_t value);

            /* (P.8.)
            If there is no device on a certain bus or if a device doesn't have multiple
            functions, it doesn't make sense to enumerate all functions for all devices. 
            It'd make device selection quite slow. We'll need a way to ask a device if it
            has functions of not.
            */
            bool DeviceHasFunctions(rexos::common::uint16_t bus, 
                rexos::common::uint16_t device);
            
            /* (P.10.)
            End goal: Driver manager will be passed to the PCI controller for it to
            communicate with it and add drivers for all the devices connected to it
            */
            void SelectDriver(rexos::drivers::DriverManager* driver_manager,
            // (P.19.) For later use 
                rexos::hardwarecomm::InterruptManager* interrupt);
            // (P.21.) A function that gives us a driver based on a device
            // descriptor and interrupt manager
            rexos::drivers::Driver* GetDriver(PCIDeviceDescriptor dev, 
                InterruptManager* interrputs);
            // (P.13.) Return a descriptor object for a device w/ give bus, device,
            // function
            PCIDeviceDescriptor GetDeviceDescriptor(rexos::common::uint16_t bus, 
                rexos::common::uint16_t device, 
                rexos::common::uint16_t function);
            // (P.18.) A function to get the BAR from bar offset in PCI 
            // configuration
            BaseAddressRegister GetBaseAddressRegister(rexos::common::uint16_t bus, 
                rexos::common::uint16_t device, 
                rexos::common::uint16_t function, 
                rexos::common::uint16_t bar);

        };
    }
}

#endif