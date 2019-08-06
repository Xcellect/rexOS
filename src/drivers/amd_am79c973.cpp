#include <drivers/amd_am79c973.h>

using namespace rexos;
using namespace rexos::drivers;
using namespace rexos::common;
using namespace rexos::hardwarecomm;

void printf(char*);
void printfHex(uint8_t);


RawDataHandler::RawDataHandler(amd_am79c973* backend) {
    this->backend = backend;
    backend->SetHandler(this);
}
RawDataHandler::~RawDataHandler() {
    backend->SetHandler(0);
}
bool RawDataHandler::OnRawDataReceived(uint8_t* buffer, uint32_t size) {
    return false;
}
void RawDataHandler::Send(uint8_t* buffer, uint32_t size) {
    backend->Send(buffer, size);
}



// (Am.1.) Constructor
amd_am79c973::amd_am79c973(PCIDeviceDescriptor* dev, InterruptManager* intManager) 
: Driver(),
InterruptHandler(dev->interrupt + 0x20, intManager), // + HW int offset
MACAddressPort0(dev->portBase),
MACAddressPort2(dev->portBase + 0x02),
MACAddressPort4(dev->portBase + 0x04),
registerDataPort(dev->portBase + 0x10),
registerAddressPort(dev->portBase + 0x12),
resetPort(dev->portBase + 0x14),
busControlRegisterDataPort(dev->portBase + 0x16)
{
    this->handler = 0;
    currentSendBuffer = 0;
    currentReceiveBuffer = 0;

    uint64_t MAC0 = MACAddressPort0.Read() % 256;
    uint64_t MAC1 = MACAddressPort0.Read() / 256;
    uint64_t MAC2 = MACAddressPort2.Read() % 256;
    uint64_t MAC3 = MACAddressPort2.Read() / 256;
    uint64_t MAC4 = MACAddressPort4.Read() % 256;
    uint64_t MAC5 = MACAddressPort4.Read() / 256;

    uint64_t MAC = MAC5 << 40
                | MAC4 << 32
                | MAC3 << 24
                | MAC2 << 16
                | MAC1 << 8
                | MAC0; 

    // Setting the device to 32bit mode
    registerAddressPort.Write(20);
    busControlRegisterDataPort.Write(0x102);

    // stop bit = related to resetting
    registerAddressPort.Write(0);
    registerDataPort.Write(0x04);

    // initBlock
    initBlock.mode = 0x0000; // prom mode = false
    initBlock.reserved1 = 0;
    initBlock.numSendBuffers = 3;
    initBlock.reserved2 = 0;
    initBlock.numReceiveBuffers = 3;
    initBlock.physicalAddress = MAC;
    initBlock.reserved3 = 0;
    initBlock.logicalAddress = 0;
    // We move the ptr 15 bytes to the right and discard the last bits to move 
    // it to 16byte aligned address
    sendBufferDesc = (BufferDescriptor*) ((((uint32_t)&sendBufferDescMemory[0]) + 15) & ~((uint32_t)0xF));
    initBlock.sendBufferDescAddress = (uint32_t)sendBufferDesc;

    receiveBufferDesc = (BufferDescriptor*) ((((uint32_t)&receiveBufferDescMemory[0]) + 15) & ~((uint32_t)0xF));
    initBlock.receiveBufferDescAddress = (uint32_t)receiveBufferDesc;

    // We've selected the memory for these descriptors
    // Now we have to set these descriptors
    for(uint32_t i = 0; i < 8; i++) {
        sendBufferDesc[i].address = (((uint32_t)&sendBuffers[i] + 15) & ~(uint32_t)0xF);
        // Setting the length of the buffer descriptor
        sendBufferDesc[i].flags = 0x7FF
                                | 0xF000;
        sendBufferDesc[i].flags2 = 0;
        sendBufferDesc[i].avail = 0;
        
        receiveBufferDesc[i].address = (((uint32_t)&receiveBuffers[i]) + 15) & ~(uint32_t)0xF;
        receiveBufferDesc[i].flags = 0xF7FF
                                | 0x80000000; // declares itself as a receive buffer
        receiveBufferDesc[i].flags2 = 0;
        sendBufferDesc[i].avail = 0;
    }   // Constructed the initialization block 
    // Now we move the init block into the device 
    registerAddressPort.Write(1);
    registerDataPort.Write((uint32_t)(&initBlock) & 0xFFFF);
    registerAddressPort.Write(2);
    registerDataPort.Write(((uint32_t)(&initBlock) >> 16) & 0xFFFF);
}
amd_am79c973::~amd_am79c973() {

}
// (Am.2.) Activate 
void amd_am79c973::Activate() {
    registerAddressPort.Write(0);
    registerDataPort.Write(0x41);   // enables the interrupts

    registerAddressPort.Write(4);
    uint32_t temp = registerDataPort.Read();    // read the 4th register
    registerAddressPort.Write(4);
    // write back on it resetting one of the bits to 1
    registerDataPort.Write(temp | 0xC00);  

    registerAddressPort.Write(0);
    registerDataPort.Write(0x42);
}
// (Am.3.) Reset
int amd_am79c973::Reset() {
    resetPort.Read();
    resetPort.Write(0);
    return 10; // wait for 10ms
}


// (Am.4.) Handle interrupt
uint32_t amd_am79c973::HandleInterrupt(uint32_t esp) {
    printf("\nINTERRUPT FROM AMD am79c973\n");
    // Similar to Programmable Interrupt Controller. If there it tells us there
    // is data, we need to fetch it
    registerAddressPort.Write(0);
    uint32_t temp = registerDataPort.Read();    // read reg 0
    // Can't do switch case bc multiple bits might be set concurrently since
    // you could have multiple errors at the same time
    if((temp & 0x8000) == 0x8000) printf("AMD am79c973 ERROR\n");
    if((temp & 0x2000) == 0x2000) printf("AMD am79c973 COLLISION ERROR\n");
    if((temp & 0x1000) == 0x1000) printf("AMD am79c973 MISSED FRAME\n");
    if((temp & 0x0800) == 0x0800) printf("AMD am79c973 MEMORY ERROR\n");
    if((temp & 0x0400) == 0x0400) Receive();
    if((temp & 0x0200) == 0x0200) printf("AMD am79c973 DATA SENT\n");
    // acknowledge
    registerAddressPort.Write(0);
    registerDataPort.Write(temp);
    
    if((temp & 0x0100) == 0x0100) printf("AMD am79c973 INIT DONE\n");
    return esp;

}

// // (Am.5.) Send and receive functions for this devices
void amd_am79c973::Send(rexos::common::uint8_t* buffer, int size) {
    // (Am.5.a.) Need to know where the data is written to <- given by currentSendBuffer
    int sendDescriptor = currentSendBuffer;
    // Move/cycle the current send buffer to the next so we can send data from
    // different tasks synchronusly
    currentSendBuffer = (currentSendBuffer + 1) % 8;
    // Do not send more than 1518 bytes. If so, there might have been error in
    // other layers
    if(size > 1518)
        size = 1518;
    // Copy the data into the current send buffer. So we set src ptr to the end
    // of the buffer data (i) we want to send and dst ptr to the end of the address
    // of send buffer where we copy this (i) buffer data. Then, we move the
    // ptrs to the left (allocating space as usual)
    for(uint8_t *src = buffer + size - 1,
                *dst = (uint8_t*) (sendBufferDesc[sendDescriptor].address + size -1);
                src >= buffer; src--, dst--) {
                    *dst = *src;    // Copy data from right to left
                    // Similar to stack operation
                }
    
    printf("Sending: ");
    for(int i = 0; i < size; i++) {
        printfHex(buffer[i]); 
        printf(" ");
    }
    sendBufferDesc[sendDescriptor].avail = 0;   // Mark not available
    sendBufferDesc[sendDescriptor].flags2 = 0;  // Clear error messages
    // See pages 186-188 of http://support.amd.com/TechDocs/20550.pdf
    sendBufferDesc[sendDescriptor].flags = 0x8300F000   // Set the size
                                        | ((uint16_t)((-size) & 0xFFF));
    registerAddressPort.Write(0);   // Setting command to 0th register
    registerDataPort.Write(0x48);   // Command 0x48 is the send command
    
}
void amd_am79c973::Receive() {
    printf("AMD am79c973 DATA RECEIVED\n");
    // Iterate through the receive buffers as long as we have receive buffers
    // that contain data
    // In the loop we'll cycle through the currentReceiveBuffer until we find
    // a receive buffer that has no data
    for( ; (receiveBufferDesc[currentReceiveBuffer].flags & 0x80000000) == 0; // checks error
            currentReceiveBuffer = (currentReceiveBuffer+1)%8 ) {
                // Inside the loop we have receive buffer that has data
                // Checks the STP and ENP. If both are 1, the frame fits into
                // a single buffer. Else, it's spread over more than one buffer
                // See page 184
                if( !(receiveBufferDesc[currentReceiveBuffer].flags & 0x40000000) &&
                    ((receiveBufferDesc[currentReceiveBuffer].flags & 0x03000000) == 0x03000000) ) {
                        uint32_t size = receiveBufferDesc[currentReceiveBuffer].flags & 0xFFF;  // read size
                        if(size > 64) // above size is of an ethernet 2 frame
                            size -= 4;  // remove the last 4 bytes of its checksum
                        // copy the address of the data
                        uint8_t* buffer = (uint8_t*) (receiveBufferDesc[currentReceiveBuffer].address);
                        
                        
                        
                        if(handler != 0) {
                            if(handler->OnRawDataReceived(buffer, size)) {
                                Send(buffer,size);
                            }
                        }
                        
                        for(int i = 0; i < (size > 64 ? 64 : size) ; i++) {
                            // Print what we received
                            printfHex(buffer[i]);
                            printf(" ");
                        }
                        /* instead of printing the received data, pass it to
                        the handler
                        for(int i = 0; i < size; i++) {
                            // Print what we received
                            printfHex(buffer[i]);
                            printf(" ");
                        }
                        */
                    }
                    // Tell the device we're done handling this
                    receiveBufferDesc[currentReceiveBuffer].flags2 = 0;
                    receiveBufferDesc[currentReceiveBuffer].flags = 0x80000F7FF; // clears the buffer
            }
} 

void amd_am79c973::SetHandler(RawDataHandler* handler) {
    this->handler = handler;
}

uint64_t amd_am79c973::GetMACAddress() {
    return initBlock.physicalAddress;
}

void amd_am79c973::SetIPAddress(common::uint32_t IP_BE) {
    initBlock.logicalAddress = IP_BE;
}

common::uint64_t amd_am79c973::GetIPAddress() {
    return initBlock.logicalAddress;
}