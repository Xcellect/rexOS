#include <filesystem/msdospart.h>
#include <filesystem/fat.h>
using namespace rexos;
using namespace rexos::common;
using namespace rexos::drivers;
using namespace rexos::filesystem;
void printf(char*);
void printfHex(uint8_t);
void MSDOSPartitionTable::ReadPartitions(rexos::drivers::ATA *hdd) {
    // Instantiate the MBR struct and read from HDD into that
    MasterBootRecord mbr;
    printf("\nMBR: ");
    hdd->Read28(0, (uint8_t*)&mbr, sizeof(MasterBootRecord));
    /*
    // S = 0x3F in the CHS address of the partion 1. Because first 63 sectors
    // are reserved
    for(int i = 0x1BE; i <= 446 + 4*16 + 1; i++) {
        printfHex(((uint8_t*)&mbr)[i]);
        printf(" ");
    }
    printf("\n");
    */
    if(mbr.magicnumber != 0xAA55) {
        printf("Illegal MBR");
        return;
    }
    
    // Inspect the partition tables
    for(int i = 0; i < 4; i++) {
        if(mbr.primaryPartition[i].partition_id == 0x00)
            continue;
        printf(" Partition ");
        printfHex(i & 0xFF);
        if(mbr.primaryPartition[i].bootable == 0x80)
            printf(" [Bootable] ");
        else
            printf(" [Not Bootable] ");
        printf("Type: ");
        printfHex(mbr.primaryPartition[i].partition_id);
        // Passing the offset of the partition to the BPD
        ReadBPD(hdd, mbr.primaryPartition[i].start_lba);
    }
    
}