#include <net/ethframe.h>

using namespace rexos;
using namespace rexos::common;
using namespace rexos::net;
using namespace rexos::drivers;

EthernetFrameHandler::EthernetFrameHandler(EthernetFrameProvider* backend, common::uint16_t etherType) {
    this->etherType_BE = ((etherType & 0x00FF) << 8)
                        | ((etherType & 0xFF00) >> 8);  // Converting LE to BE
    this->backend = backend;
    // Register this handler in the ethernet frame provider
    backend->handlers[etherType_BE] = this;
}
EthernetFrameHandler::~EthernetFrameHandler() {
    if(backend->handlers[etherType_BE] == this)
        backend->handlers[etherType_BE] = 0;
}

bool EthernetFrameHandler::OnEthernetFrameReceived(common::uint8_t* etherFramePayload, uint32_t size) {
    return false;
}
void EthernetFrameHandler::Send(common::uint64_t dstMAC_BE, common::uint8_t* payloadData,
                                                    common::uint32_t size) {
    backend->Send(dstMAC_BE, etherType_BE, payloadData, size);
}


EthernetFrameProvider::EthernetFrameProvider(drivers::amd_am79c973* backend) 
: RawDataHandler(backend)
{   
    for(uint32_t i = 0; i < 65535; i++) {
        handlers[i] = 0;
    }
}
EthernetFrameProvider::~EthernetFrameProvider() {}




// When we receive the raw data, we put the ethernet header structure over it
bool EthernetFrameProvider::OnRawDataReceived(common::uint8_t* rawData, common::uint32_t size) {
    
    if(size < sizeof(EthernetFrameHeader)) {
        return false;
    }
    EthernetFrameHeader* header = (EthernetFrameHeader*)rawData;
    bool sendBack = false;
    if(header->dstMAC_BE == 0xFFFFFFFFFFFF
    || header->dstMAC_BE == backend->GetMACAddress()) {
        if(handlers[header->etherType_BE] != 0) {
            // Take only the data inside the frame
            sendBack = handlers[header->etherType_BE]->OnEthernetFrameReceived(
                rawData + sizeof(EthernetFrameHeader), 
                size - sizeof(EthernetFrameHeader));
        }
        // Removed the last 4bytes of C2C checksum in the net driver already
    }

    if(sendBack) {
        header->dstMAC_BE = header->srcMAC_BE;
        header->srcMAC_BE = backend->GetMACAddress();
    }
    return sendBack;
}
// Here we'll need the dynamic memory management (heap)
void EthernetFrameProvider::Send(common::uint64_t dstMAC_BE, common::uint16_t etherType_BE, 
                        common::uint8_t* buffer, common::uint32_t size) {
    // Allocate the memory for header and size of the buffer we want to send
    uint8_t* bufferToSend = (uint8_t*)MemoryManager::activeMemoryManager->malloc(sizeof(EthernetFrameHeader) + size);
    // This header now points the bufferToSend
    EthernetFrameHeader* header = (EthernetFrameHeader*) bufferToSend;
    header->dstMAC_BE = dstMAC_BE;
    header->srcMAC_BE = backend->GetMACAddress();
    header->etherType_BE = etherType_BE;

    // Now copy the data from arg buffer to bufferToSend
    uint8_t* src = buffer;
    uint8_t* dst = bufferToSend + sizeof(EthernetFrameHeader);
    for(uint32_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    backend->Send(bufferToSend, size + sizeof(EthernetFrameHeader));
    MemoryManager::activeMemoryManager->free(bufferToSend);
}

common::uint32_t EthernetFrameProvider::GetIPAddress() {
    return backend->GetIPAddress();
}

common::uint64_t EthernetFrameProvider::GetMACAddress() {
    return backend->GetMACAddress();
}