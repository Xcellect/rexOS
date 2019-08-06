#ifndef __REXOS__DRIVERS__AMD_AM79C973
#define __REXOS__DRIVERS__AMD_AM79C973

#include <common/types.h>
#include <hardwarecomm/interrupts.h>
#include <hardwarecomm/pci.h>
#include <hardwarecomm/port.h>
#include <drivers/driver.h>


namespace rexos {
    namespace drivers {
        class amd_am79c973;
        // Instead of deriving the network chip from an ethernet driver, which
        // would give us the raw data handler, we'll be imposing it here like 
        // the mouse and keyboard's instance.
        // The RawDataHandler in the ethernet driver class should have an
        // ethernet driver backend, because unlike with the mouse and keyboard
        // we want to send data to the device. So the handler needs to know its
        // backend where it's getting its data from.
        class RawDataHandler {
            protected:
                amd_am79c973* backend;  // This should be a ptr to the ethernet driver
            public:
                RawDataHandler(amd_am79c973* backend);
                ~RawDataHandler();

                virtual bool OnRawDataReceived(common::uint8_t* buffer, common::uint32_t size);
                void Send(common::uint8_t* buffer, common::uint32_t size);
        };

        class amd_am79c973 : public Driver, public hardwarecomm::InterruptHandler {
                struct InitializationBlock {
                    common::uint16_t mode; // for promiscuous mode
                    // 4 reserved bits
                    unsigned reserved1 : 4;
                    // # of receive and send buffers which are 4 bits
                    // We'll write 3 in there and that will mean we have 
                    // 8 buffers (?)
                    unsigned numSendBuffers : 4;
                    unsigned reserved2 : 4;
                    unsigned numReceiveBuffers : 4;
                    // MAC is 48 bits but treated like 64bit int
                    common::uint64_t physicalAddress : 48;
                    common::uint16_t reserved3;
                    common::uint64_t logicalAddress;
                    common::uint32_t receiveBufferDescAddress;
                    common::uint32_t sendBufferDescAddress;
                } __attribute__((packed));
                
                
                // BufferDecriptor is a nested struct so its scope is unique
                struct BufferDescriptor {
                    common::uint32_t address;
                    common::uint32_t flags;
                    common::uint32_t flags2;
                    common::uint32_t avail;
                }  __attribute__((packed));
                
                
                
                // 3 16bit ports to read the MAC address, each gives 2 bytes of MAC
                hardwarecomm::Port16Bit MACAddressPort0;
                hardwarecomm::Port16Bit MACAddressPort2;
                hardwarecomm::Port16Bit MACAddressPort4;
                hardwarecomm::Port16Bit registerDataPort;
                hardwarecomm::Port16Bit registerAddressPort;
                hardwarecomm::Port16Bit resetPort;
                hardwarecomm::Port16Bit busControlRegisterDataPort;
                // InitializationBlock holds a ptr to the array of
                // BufferDescriptors which hold the pointers to the buffers
                InitializationBlock initBlock;
                // 1. 16byte aligned buffers: 8byte for send, 8byte for receive
                // Each is an array of 8 buffers of size 2KiB + 15 byte for
                // alignment. 
                // 2. Also, need some memory for the descriptors (remember GDT)
                // for these buffers. They also need to be 16 bytes aligned.
                // 3. Then we'll move a ptr to a buffer descriptor into the
                // memory
                // 4. We need a ptr to define which of these 8 send and receive
                // buffers is/are active
                BufferDescriptor* sendBufferDesc;                       // 3
                common::uint8_t sendBufferDescMemory[2048 + 15];        // 2
                common::uint8_t sendBuffers[2*1024 + 15][8];            // 1
                common::uint8_t currentSendBuffer;

                BufferDescriptor* receiveBufferDesc;                    // 3
                common::uint8_t receiveBufferDescMemory[2048 + 15];     // 2
                common::uint8_t receiveBuffers[2*1024 + 15][8];         // 1
                common::uint8_t currentReceiveBuffer;
                // In the driver for the network device, we'll implement this
                // handler
                RawDataHandler* handler;
            public:
                amd_am79c973(hardwarecomm::PCIDeviceDescriptor* dev, hardwarecomm::InterruptManager* intManager);
                ~amd_am79c973();
                void Activate();
                int Reset();
                rexos::common::uint32_t HandleInterrupt(rexos::common::uint32_t esp);

                void Send(rexos::common::uint8_t* buffer, int count);
                void Receive();
                void SetHandler(RawDataHandler* handler);
                rexos::common::uint64_t GetMACAddress();
                // Instead of using DHCP, we set the IP address manually
                void SetIPAddress(common::uint32_t IP_BE);
                common::uint64_t GetIPAddress();
        };
    }


}



#endif