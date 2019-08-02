#ifndef __REXOS__DRIVERS__ATA_H
#define __REXOS__DRIVERS__ATA_H
#include <hardwarecomm/port.h>


namespace rexos {
    namespace drivers {
        // Advanced Technology Attachment
        class ATA {
            protected:
                // When we communicate with the hard driver, the controller
                // will need 9 ports
                // First one is 16bit
                hardwarecomm::Port16Bit dataPort;   // R/W: data 
                // Rest are 8bit
                hardwarecomm::Port8Bit errorPort;   // R: error messages
                hardwarecomm::Port8Bit sectorCountPort; // # of sector to read
                // Following 3 ports are used to transmit the logical block
                // address of the sector we want to read
                hardwarecomm::Port8Bit lbaLowPort;
                hardwarecomm::Port8Bit lbaMidPort;
                hardwarecomm::Port8Bit lbaHighPort;
                hardwarecomm::Port8Bit devicePort;    // Master/Slave selector
                hardwarecomm::Port8Bit commandPort;          // For instructions
                hardwarecomm::Port8Bit controlPort;          // For status msgs
                // 2 hard drives on the same bus. Master/slaves are simply
                // distinguishing words for them. In early days, you could
                // only boot from master hard drives
                bool master;
                common::uint16_t bytesPerSector;
            public:
                // We have multiple ATA buses
                ATA(common::uint16_t portBase, bool master);
                ~ATA();
                // For asking if the hard drive is there, what kind etc.
                void Identify();
                // Highest bits of this sector needs to be ignored
                void Read28(common:: uint32_t sector, int count);
                void Write28(common::uint32_t sector, common::uint8_t* data, int count);
                // A function to flush the cache of the hard drive. 
                // See written notes
                void Flush();
        };
    }
}

#endif