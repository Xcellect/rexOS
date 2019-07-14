#include <hardwarecomm/pci.h>
using namespace rexos::common;
using namespace rexos::hardwarecomm;
using namespace rexos::drivers;


PCIDeviceDescriptor::PCIDeviceDescriptor() {

}
PCIDeviceDescriptor::~PCIDeviceDescriptor() {
    
}

// Constructor
PCIController::PCIController()
: dataport(0xCFC),  // (P.2.) Setting port numbers for data and command ports
commandport(0xCF8)
{

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
void PCIController::SelectDriver(DriverManager* driver_manager) {
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
// (P.14.)
PCIDeviceDescriptor PCIController::GetDeviceDescriptor(uint16_t bus, 
          uint16_t device, 
          uint16_t function) {
    PCIDeviceDescriptor result;
    result.bus = bus;
    result.device = device;
    result.function = function;
    
    result.vendor_id = Read(bus, device, function, 0x0);
    result.device_id = Read(bus, device, function, 0x2);

    result.class_id = Read(bus, device, function, 0xb);
    result.subclass_id = Read(bus, device, function, 0x0a);
    result.interface_id = Read(bus, device, function, 0x09);

    result.revision = Read(bus, device, function, 0x08);
    result.interrupt = Read(bus, device, function, 0x3c);
    return result;
}