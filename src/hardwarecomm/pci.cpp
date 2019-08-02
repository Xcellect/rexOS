#include <hardwarecomm/pci.h>
#include <drivers/amd_am79c973.h>

using namespace rexos::common;
using namespace rexos::drivers;
using namespace rexos::hardwarecomm;


PCIDeviceDescriptor::PCIDeviceDescriptor() {
}

PCIDeviceDescriptor::~PCIDeviceDescriptor() {
}

// Constructor
PCIController::PCIController()
: dataport(0xCFC),  // (P.2.) Setting port numbers for data and command ports
commandport(0xCF8) {
}


PCIController::~PCIController() {
}


// (P.3.) Constructing the idenfier we'll send  to the PCI controller from
// passed bus, device, and function numbers
uint32_t PCIController::Read(uint16_t bus, uint16_t device, uint16_t function, uint16_t registeroffset) {
    // (P.4.) Set up the id with proper alignment
    uint32_t id =
        0x1 << 31   // First bit will explicitly be 1
        | ((bus & 0xFF) << 16)  // bus left shifted by 16 bits
        | ((device & 0x1F) << 11) // device left shifted by 11 bits
        | ((function & 0x07) << 8)  // function left shifted by 8 bits
        | (registeroffset & 0xFC);  // register offset isn't shifted
        // Bitwise AND with 0xFC means cutting off last 2 bits of this number.
        // registeroffsetnumbers are 4 bytes aligned. This is actually an 
        // address of a byte. Since these bytes are grouped as 32bit ints
        // we would ask for a location of an int. We can't just get the 3rd or
        // 4th byte of the integer, we can only get the entire integer in one
        // piece.
        /*  Eg. Data chunk
                     _ _ _ _
                    | | | | |
                     4 5 6 7
        If we want the data at location 6, first we'll have to get that 4 byte
        chunk by sending the location of that chunk (4). Then, we extract the
        byte at 6 out of the result.
        */
    // (P.5.) Pass this id to the command port
    commandport.Write(id);
    // (P.6.) Read the data from the data port
    uint32_t result = dataport.Read();
    return result >> (8 * (registeroffset % 4));
}

// (P.7.) Contstructed identifier from P.3. will be written to the command port
// Then, unlike before we don't read from the data port. Rather we write to the
// data port.
void PCIController::Write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset, uint32_t value) {
    uint32_t id =
        0x1 << 31   // First bit will explicitly be 1
        | ((bus & 0xFF) << 16)  // bus left shifted by 16 bits
        | ((device & 0x1F) << 11) // device left shifted by 11 bits
        | ((function & 0x07) << 8)  // function left shifted by 8 bits
        | (registeroffset & 0xFC);  // discarding last 2 bits
    commandport.Write(id);
    dataport.Write(value);
}

// (P.9.) To find this out, we read the bit w/ address 0x0E of func 0
// Speeds up enumeration of devices by a factor of 8
bool PCIController::DeviceHasFunctions(uint16_t bus, 
          uint16_t device) {
    return Read(bus, device, 0, 0x0E) & (1 << 7);   // Only need the 7th bit
}
void printf(char* str);
void printfHex(uint8_t);
// (P.11.) This function: 
// 1. Enumerates bus, device, function numbers to find devices
// 2. Selects drivers based on vendor, device, class, subclass id of devices
// 3. Adds those drivers to the passed driver manager
void PCIController::SelectDriver(DriverManager* driver_manager, rexos::hardwarecomm::InterruptManager* interrupt) {
    for( int bus = 0; bus < 8; bus++) {
        for(int device = 0; device < 32; device++) {
            // If device has functions, set num to 8, else set it to 1
            int dev_functions = DeviceHasFunctions(bus, device) ? 8 : 1;
            for(int function = 0; function < dev_functions; function++) {
                // (P.15)
                PCIDeviceDescriptor dev = GetDeviceDescriptor(bus, device, function);
                // There can be gaps between functions. eg. function 2 then
                // using a break would prevent us from finding that
                if(dev.vendor_id == 0x0000 || dev.vendor_id == 0xFFFF) {
                    continue;  // keep looking
                }
                // (P.20.) Iterate the BARs inside this loop
                for(int barNum = 0; barNum < 6; barNum++) {
                    BaseAddressRegister bar = 
                        GetBaseAddressRegister(bus, device, function, barNum);
                    // Proceed only if the address is set from above function
                    if(bar.address && (bar.type == InputOutput))    // I/O type
                    // The address will be set to the higher bits of the BAR,
                    // which in case of I/O BARs contains the port number
                        dev.portBase = (uint32_t)bar.address;
                }
                // Get driver for this dev
                // We only want one driver for a device and not every base
                // address register
                Driver* driver = GetDriver(dev, interrupt);
                if(driver != 0)
                    driver_manager->AddDriver(driver);

                printf("PCI BUS ");
                printfHex(bus & 0xFF);

                printf(", DEVICE ");
                printfHex(device & 0xFF);

                printf(", FUNCTION ");
                printfHex(function & 0xFF);

                printf(" = VENDOR ");
                printfHex((dev.vendor_id & 0xFF00) >> 8);
                printfHex(dev.vendor_id & 0xFF);
                
                printf(", DEVICE ");
                printfHex((dev.device_id & 0xFF00) >> 8);
                printfHex(dev.device_id & 0xFF);
                printf("\n");
           }
        }
    }
}
// (P.23.)
BaseAddressRegister PCIController::GetBaseAddressRegister(uint16_t bus, uint16_t device,
                uint16_t function, uint16_t bar) {
    BaseAddressRegister result;
    // taking first 7 bits of the BAR at offset 0x0E
    uint32_t headertype = Read(bus, device, function, 0x0E) & 0x7F;
    // iterating 6 BARs if they're 32bit, 2 BARs if they're 64bit
    int maxBARs = 6 - (4*headertype);
    // if the requested bar is greater than the max, return the result 
    if(bar >= maxBARs)
        return result;  // address is null here
    // Read the offset at 0x10 + 4*bar. 4 bc BARs have a size of 4
    uint32_t bar_value = Read(bus,device,function, 0x10 + 4*bar);
    // Set type
    result.type = (bar_value & 0x1) ? InputOutput : MemoryMapping;
    uint32_t temp;
    
    if(result.type == MemoryMapping) {
        //uint16_t barOffset = 0x10 + 4*bar;
        /*
        // google lowlevel.eu/pci/base_analyse
        result.prefetchable = ((bar_value >> 3) & 0x1) == 0x1;
        // Right-shift 1 place to discard the LSB 0 bit, then take the LSB 1
        // and LSB 2 bits by ANDing with 0x3
        switch ((bar_value >> 1) & 0x3)
        {
            case 0: // 32bit
                {
                    Write(bus,device,function,barOffset,0xFFFFFFF0);
                    bar_value = Read(bus, device, function, barOffset);
                    if(bar_value == 0) {
                        if(result.prefetchable == true) {
                            printf("ERROR: 32Bit Memory BAR ");
                            printfHex(bar);
                            printf(" contains no writable address bits\n");
                        }
                        printf("BAR ");
                        printfHex(bar);
                        printf("is unused\n");
                    } else {
                        result.address = (uint8_t*) (bar_value & ~0x5);
                    }
                    break;
                }
            case 1: // 20bit
                {
                    if(headertype == 0x01) {
                        printf("ERROR: 20Bit Memory BAR ");
                        printfHex(bar);
                        printf(" is not allowed for a bridge\n");
                    }
                    Write(bus,device,function,barOffset,0xFFFFFFF0);
                    bar_value = Read(bus, device, function, barOffset);
                    if(bar_value == 0) {
                        if(result.prefetchable == true) {
                            printf("ERROR: 20Bit Memory BAR ");
                            printfHex(bar);
                            printf(" contains no writable address bits\n");
                        }
                        printf("BAR ");
                        printfHex(bar);
                        printf("is unused\n");
                    } else {
                        result.address = (uint8_t*) (bar_value & ~0x5);
                    }
                    break;
                }
            case 2: // 64bit
                {
                    if( bar >= maxBARs-1) {
                        printf("ERROR: 64Bit Memory BAR ");
                        printfHex(bar);
                        printf(" can not start at last position\n");
                    }
                    if(result.prefetchable == false) {
                        printf("ERROR: 64Bit Memory BAR ");
                        printfHex(bar);
                        printf(" contains a non prefetchable resource\n");
                    }
                    Write(bus,device,function,barOffset,0xFFFFFFF0);
                    Write(bus,device,function,barOffset + 4,0xFFFFFFF0);
                    const uint32_t bar_low_value = Read(bus,device,function,barOffset) & 0xFFFFFFF0;
                    const uint32_t bar_high_value = Read(bus,device,function,barOffset + 4);
                    result.address = (uint8_t*) (bar_low_value & ~0x5);
                    break;
                }
            default:
                printf ("ERROR: Memory-BAR ");
                printfHex(bar);
                printf(" is invalid");
                break;
        }
        */
    } else {    // I/O
        // Discard bits at LSB 0 (type) and 1 (reserved) of BAR value. Rest are
        // the port bits address we need for port
        result.address = (uint8_t*) (bar_value & ~0x3);
        result.prefetchable = false;
    }
    return result;
}


// (P.22.)
Driver* PCIController::GetDriver(PCIDeviceDescriptor dev, InterruptManager* interrputs) {
    Driver *driver = 0;
    switch(dev.vendor_id) {
        case 0x1022: // AMD
            switch(dev.device_id) {
                case 0x2000: // am79c973 network chip
                printf("AMD am79c973 ");
                        driver = (amd_am79c973*)MemoryManager::activeMemoryManager->malloc(sizeof(amd_am79c973));
                    if(driver != 0) {
                        new (driver)amd_am79c973(&dev, interrputs);
                    } else
                        printf("instantiation failed");
                   
                    return driver;
                    break;
            }
            break;
        case 0x8086: // Intel
            break;
    }
    switch(dev.class_id) {
        case 0x03: // Graphics
            switch(dev.subclass_id) {
                case 0x00:  // VGA graphics devices
                    printf("VGA ");
                    break;
            }
            break;
    }
    return driver;
}

// (P.14.)
PCIDeviceDescriptor PCIController::GetDeviceDescriptor(uint16_t bus, 
          uint16_t device, 
          uint16_t function) {
    PCIDeviceDescriptor result;
    result.bus = bus;
    result.device = device;
    result.function = function;
    
    result.vendor_id = Read(bus, device, function, 0x00);
    result.device_id = Read(bus, device, function, 0x02);

    result.class_id = Read(bus, device, function, 0x0b);
    result.subclass_id = Read(bus, device, function, 0x0a);
    result.interface_id = Read(bus, device, function, 0x09);

    result.revision = Read(bus, device, function, 0x08);
    result.interrupt = Read(bus, device, function, 0x3c);
    return result;
}