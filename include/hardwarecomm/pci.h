#ifndef __REXOS__HARDWARECOMM__PCI_H
#define __REXOS__HARDWARECOMM__PCI_H
#include <hardwarecomm/port.h>
#include <drivers/driver.h>
#include <common/types.h>
#include <hardwarecomm/interrupts.h>   
namespace rexos {
    namespace hardwarecomm {
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
            void SelectDriver(rexos::drivers::DriverManager* driver_manager);
            // (P.13.) Return a descriptor object for a device w/ give bus, device,
            // function
            PCIDeviceDescriptor GetDeviceDescriptor(rexos::common::uint16_t bus, 
            rexos::common::uint16_t device, 
            rexos::common::uint16_t function);
        };
    }
}

#endif