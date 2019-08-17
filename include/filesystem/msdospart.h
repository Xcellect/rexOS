#ifndef __REXOS__FILESYSTEM__MSDOSPART_H
#define __REXOS__FILESYSTEM__MSDOSPART_H
#include <common/types.h>
#include <drivers/ata.h>

namespace rexos {
    namespace filesystem {
        struct PartitionTableEntry {
            common::uint8_t bootable;   // bootable flag
            // CHS (Cyllinder, Head, Sector) start
            common::uint8_t start_head;
            common::uint8_t start_sector: 6;
            common::uint16_t start_cyllinder: 10;
            
            common::uint8_t partition_id;
            // CHS (Cyllinder, Head, Sector) end
            common::uint8_t end_head;
            common::uint8_t end_sector: 6;
            common::uint16_t end_cyllinder: 10;
            // Point of interest
            // LBA: Logical Block Address
            // FAT32 adds the following value to the offsets it's reading
            common::uint32_t start_lba; // The offset we have to add in the FS
            common::uint32_t length;
        } __attribute__((packed));
        struct MasterBootRecord {
            common::uint8_t bootloader[440];    // 440 bytes boot strap/loader
            common::uint32_t signature;
            common::uint16_t unused; // 2 unused bytes
            PartitionTableEntry primaryPartition[4];
            common::uint16_t magicnumber;
        } __attribute__((packed));
        
        class MSDOSPartitionTable {
            public:
                static void ReadPartitions(rexos::drivers::ATA *hdd);

        };
    }
}
#endif