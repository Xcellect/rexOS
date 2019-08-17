#include <filesystem/fat.h>
using namespace rexos;
using namespace rexos::common;
using namespace rexos::drivers;
using namespace rexos::filesystem;
void printf(char*);
void printfHex(uint8_t);
void filesystem::ReadBPD(rexos::drivers::ATA* hdd, common::uint32_t partitionOffset) {
    // Similar to MSDOSPartitionTable, we'll instantiate a BPD here
    BiosParameterBlockFAT32 bpb;
    // Read the first sector of the partition given by the partitionOffset
    hdd->Read28(partitionOffset, (uint8_t*)&bpb, sizeof(BiosParameterBlockFAT32));
    /*
    for(int i = 0x00; i <= sizeof(BiosParameterBlockFAT32); i++) {
        printfHex(((uint8_t*)&bpb)[i]);
        printf(" ");
    }
    */
    /*
    printf("Sectors per cluster: ");
    printfHex(bpb.sectorsPerCluster);
    printf("\n");
    */

    uint32_t FATStart = partitionOffset + bpb.reservedSectors;
    uint32_t FATSize = bpb.tableSize;
    
    uint32_t dataStart =  FATStart + bpb.FATCopies * FATSize;
    // rootCluster has an offset of 2 which needs to be negated
    uint32_t rootStart = dataStart + bpb.sectorsPerCluster * (bpb.rootCluster - 2);

    DirectoryEntryFAT32 dirent[16];
    hdd->Read28(rootStart, (uint8_t*)&dirent[0], 16*sizeof(DirectoryEntryFAT32));
    for(int i = 0; i < 16; i++) {
        // If the name of an entry starts w/ \0 (nullbyte), there's no more
        if(dirent[i].name[0] == 0x00)
            break;
        // Skip the illegal value for attribute bytes (lookup: MS-Patent-2012)
        if((dirent[i].attributes == 0x0F) == 0x0F)
            continue;
        char* foo = "        \n";
        for(int j = 0; j < 8; j++) {
            // Since filenames don't have \n, copy the filename in this buffer
            foo[j] = dirent[i].name[j];
        }
        printf(foo);
        // If the 5th bit of the attributes is 1, it's a dir
        if((dirent[i].attributes & 0x10) == 0x10)
            continue;
        
        uint32_t firstFileCluster = (uint32_t)dirent[i].firstClusterHi << 16
                            |   (uint32_t)dirent[i].firstClusterLo;
        
        int32_t SIZE = dirent[i].size;
        int32_t nextFileCluster = firstFileCluster;
        uint8_t buffer[513];
        uint8_t FATBuffer[513];
        // Allows reading files > sector and files < cluster
        while(SIZE > 0) {
            // First sector of the file. Only allows reading files < sector
            uint32_t fileSector = dataStart + bpb.sectorsPerCluster * (nextFileCluster - 2);
            int sectorOffset = 0;
            
            // Inside this loop, we read the current sector. Here, we read the
            // cluster 1 sector (512 bytes) at a time decreasing the sector size
            // each iteration until the beginning of the data or the end of the
            // cluster is reached 
            for( ; SIZE > 0; SIZE -=512) {
                hdd->Read28(fileSector + sectorOffset, buffer, 512);
                
                /*
                1. Here, buffer index was set to the size received from dirent,
                this is bad because this might be somewhere in the RAM. So we
                set it to the remaining SIZE (which is dirent[i].size % 512), 
                not the total file size (dirent[i].size) every iteration.
                buffer[dirent[i].size > 512 ? 512 : dirent[i].size] = '\0';
                */
                buffer[SIZE > 512 ? 512 : SIZE] = '\0';
                printf((char*)buffer);
                // 3. Must only be greater (not greater or equal). Previously,
                // this was between Read and buffer. Because of this, the last
                // sector of the cluster was skipped
                if(++sectorOffset > bpb.sectorsPerCluster)
                    break;
            }
            // Another implementation choice: having a template for larger data
            // and using its size instead of uint32_t
            uint32_t FATSectorForCurrCluster = nextFileCluster / (512/sizeof(uint32_t));
            // Optional: Check if the FATBuffer already contains what we're
            // looking for. Stop looking in that case.
            hdd->Read28(FATStart + FATSectorForCurrCluster, FATBuffer, 512);
            
            
            uint32_t FATCurrClusterOffsetInSector = nextFileCluster % (512/sizeof(uint32_t));
            
            // In the next iteration of the while loop, we go to the first
            // sector of the next cluster. Turning the FATBuffer into an array
            // of integers and reading the FATCurrClusterOffsetInSector there
            nextFileCluster = ((uint32_t*)
                &FATBuffer)[FATCurrClusterOffsetInSector] & 0x0FFFFFFF;
            /* 
            2. There has been a break here. We were only reading the first
            cluster and everything after that was skipped
            
            3. Finally, when we read the nextFileCluster number, we take
            a bitwise & with 32 bit int with all F's except the highest
            nibble (halfbyte/4bit) because FAT32 only uses 28 bits for
            addressing the clusters. So, there could be some bits in that
            highest nibble which are used for something else and we don't
            want to F with that. Therefore, no F there.
            */
        }
    }
    
}