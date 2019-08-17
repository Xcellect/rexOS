#include <drivers/ata.h>


using namespace rexos;
using namespace rexos::common;
using namespace rexos::drivers;
using namespace rexos::hardwarecomm;
void printf(char*);
ATA::ATA(uint16_t portBase, bool master)
: dataPort(portBase),
  errorPort(portBase + 1), 
  sectorCountPort(portBase + 2),
  lbaLowPort(portBase + 3),
  lbaMidPort(portBase + 4),
  lbaHighPort(portBase + 5),
  devicePort(portBase + 6), 
  commandPort(portBase + 7),
  controlPort(portBase + 0x206)
{
 bytesPerSector = 512;
 this->master = master;
}
ATA::~ATA() {

}
// For asking if the hard drive is there, what kind etc.
void ATA::Identify() {
    devicePort.Write(master ? 0xA0 : 0xB0);
    controlPort.Write(0);

    devicePort.Write(0xA0);
    // Read the status of the master. Reading status requires you to talk to
    // the master and not the slave
    uint8_t status = commandPort.Read();
    if(status == 0xFF)  // no device
        return;
    devicePort.Write(master ? 0xA0 : 0xB0);
    sectorCountPort.Write(0);
    lbaLowPort.Write(0);
    lbaMidPort.Write(0);
    lbaHighPort.Write(0);
    commandPort.Write(0xEC);    // identify

    status = commandPort.Read();
    if(status == 0x00)
        return; // no device

    // Might take some for the hard drive to reply to the identify command
    // So wait until the device is ready
    while(((status & 0x80) == 0x80) && 
            ((status & 0x01) != 0x01))
            status = commandPort.Read();
    if(status & 0x01) {
        printf("ERROR");
        return;
    }
    // If we reach this line, the data is now ready to read
    // 256 bc we're reading 2 byte at a time and there are 512 to read
    for(uint16_t i = 0; i < 256; i++) {
        uint16_t data = dataPort.Read();
        char* foo = "  \0";
        foo[1] = (data >> 8) & 0x00FF;
        foo[0] = data & 0x00FF;
        printf(foo);
    }
    printf("\n");
}
// Highest bits of this sector needs to be ignored
void ATA::Read28(common:: uint32_t sector, common::uint8_t* data, int count) {
     // You cannot Read to a sector larger than what you can address with 28bits
    if(sector & 0xF0000000)     // Check if first 4 bits are 0
        return;
    if(count > bytesPerSector)
        return;

    // When we select master or slave, we have to select E0 or F0
    // In 28bit mode, we have 3 8bit ports to write, but will have
    // 4bits left from the address so we put these into the device
    // port as well
    devicePort.Write(master ? 0xE0 : 0xF0 | ((sector & 0x0F000000) >> 24));
    errorPort.Write(0);     // Clear previous error messages
    sectorCountPort.Write(1);   // For now, we'll always R/W a single sector

    // Split the sector number and put it into these 3 ports
    lbaLowPort.Write( sector & 0x000000FF );
    lbaMidPort.Write((sector & 0x0000FF00) >> 8);
    lbaHighPort.Write((sector & 0x00FF0000) >> 16);
    commandPort.Write(0x20);       // Read command

    // The hard drive might take some time until it's ready to give the data
    uint8_t status = commandPort.Read();
    while(((status & 0x80) == 0x80) && 
            ((status & 0x01) != 0x01)) { // Waits for the device to be ready
            status = commandPort.Read();
    }
    if(status & 0x01) {
        printf(" [ERROR] ");
        return;
    }

    printf(" [READING] ");
    // Reading the data
    for(uint16_t i = 0; i < count; i+=2) {
        uint16_t wdata = dataPort.Read();
        /*
        // Printing
        char* foo = "  \0";
        foo[1] = (wdata >> 8) & 0x00FF;
        foo[0] = wdata & 0x00FF;
        printf(foo);
        */
        data[i] = wdata & 0x00FF; // Writing high bytes
        // Next we write the low byte to the buffer
        if(i+1 < count) 
            data[i+1] = (wdata >> 8) & 0x00FF;
    }
    // Read the rest of the sector and discard it
    for(uint16_t i = count + (count % 2); i< bytesPerSector; i+=2) {
        dataPort.Read(); 
    }
}
void ATA::Write28(common::uint32_t sector, common::uint8_t* data, int count) {
    // You cannot write to a sector larger than what you can address with 28bits
    if(sector & 0xF0000000)     // Check if first 4 bits are 0
        return;
    if(count > bytesPerSector)
        return;

    // When we select master or slave, we have to select E0 or F0
    // In 28bit mode, we have 3 8bit ports to write, but will have
    // 4bits left from the address so we put these into the device
    // port as well
    devicePort.Write(master ? 0xE0 : 0xF0 | ((sector & 0x0F000000) >> 24));
    errorPort.Write(0);     // Clear previous error messages
    sectorCountPort.Write(1);   // For now, we'll always R/W a single sector

    // Split the sector number and put it into these 3 ports
    lbaLowPort.Write( sector & 0x000000FF );
    lbaMidPort.Write((sector & 0x0000FF00) >> 8);
    lbaHighPort.Write((sector & 0x00FF0000) >> 16);
    commandPort.Write(0x30);        // Write command

    printf("[WRITING] ");

    // Only writing the number of bytes in data
    for(uint16_t i = 0; i < count; i+=2) {
        uint16_t wdata = data[i];   // take the ith byte of the data
        if(i+1 < count)             // if the next byte is also there
            wdata |= ((uint16_t)data[i+1]) << 8;
        char* foo = "  \0";
        foo[1] = (wdata >> 8) & 0x00FF;
        foo[0] = wdata & 0x00FF;
        printf(foo); // prints the data before we send it
        dataPort.Write(wdata);
    }
    // If count is odd, add the mod 2 value (remainder) of that     
    // Basically always add 1 if odd bc the data[1] value has
    // already been written with an empty value in the previous
    // loop's 2nd line
    for(uint16_t i = count + (count % 2); i< bytesPerSector; i+=2) {
        dataPort.Write(0x0000); 
    }
}
    
// A function to flush the cache of the hard drive. 
// See written notes
void ATA::Flush() {
    devicePort.Write(master ? 0xE0 : 0xF0);
    commandPort.Write(0xE7);    // Flush


    uint8_t status = commandPort.Read();
   
    
    // Wait while the device is flushing
    while(((status & 0x80) == 0x80) && 
            ((status & 0x01) != 0x01))
            status = commandPort.Read();
    if(status & 0x01) {
        printf("ERROR");
        return;
    }
   
}