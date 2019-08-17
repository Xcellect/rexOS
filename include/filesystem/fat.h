#ifndef __REXOS__FILESYSTEM__FAT_H
#define __REXOS__FILESYSTEM__FAT_H
#include <common/types.h>
#include <drivers/ata.h>
namespace rexos {
    namespace filesystem {
        struct BiosParameterBlockFAT32 {
            common::uint8_t jump[3];
            common::uint8_t softName[8]; // mkfs.vfat
            common::uint16_t bytesPerSector;
            common::uint8_t sectorsPerCluster;  // used for calculating offsets
            common::uint16_t reservedSectors;   // ptr down to the FAT
            common::uint8_t FATCopies;          // copies of FAT
            common::uint16_t rootDirEntries;    // 0 (ignored)
            common::uint16_t totalSectors;
            common::uint8_t mediaType;
            common::uint16_t FAT16SectorCount;  // 0 (ignored)
            common::uint16_t sectorsPerTrack;
            common::uint16_t headCount;
            common::uint32_t hiddenSectors;
            common::uint32_t totalSectorCount;
            
            common::uint32_t tableSize;
            common::uint16_t extFlags;
            common::uint16_t FATVersion;
            common::uint32_t rootCluster; // this * sectors/Cluster = *root dir
            common::uint16_t FATInfo;
            common::uint16_t backupSector;
            common::uint8_t reserved0[12];
            common::uint8_t driveNumber;
            common::uint8_t reserved;
            common::uint8_t bootSignature;
            common::uint32_t volumeID;
            common::uint8_t volumeLabel[11];
            common::uint8_t FATTypeLabel[8];
        }__attribute__((packed));
        struct DirectoryEntryFAT32 {
            common::uint8_t name[8];
            common::uint8_t extension[3];
            common::uint8_t attributes;
            common::uint8_t reserved;
            common::uint8_t cTimeTenth; // c = creation
            common::uint16_t cTime;
            common::uint16_t cDate;
            common::uint16_t aTime; // a = access
            common::uint16_t firstClusterHi;
            common::uint16_t wTime;
            common::uint16_t wDate;
            common::uint16_t firstClusterLo;
            common::uint32_t size;
        }__attribute__((packed));
        
        void ReadBPD(rexos::drivers::ATA* hdd, common::uint32_t partitionOffset);
    }
}
#endif