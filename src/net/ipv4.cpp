#include <net/ipv4.h>
#include <common/types.h>
using namespace rexos;
using namespace rexos::common;
using namespace rexos::drivers;
using namespace rexos::net;
void printf(char*);
void printfHex(uint8_t);
IPv4Handler::IPv4Handler(IPv4Provider* backend, common::uint8_t protocol) {
    this->backend = backend;
    this->ip_protocol = protocol;
    // Similar to etherframe, we store ourselves in the handler
    backend->handlers[protocol] = this;
}
IPv4Handler::~IPv4Handler() {
    if(backend->handlers[ip_protocol] == this)
        backend->handlers[ip_protocol] = 0;
}
bool IPv4Handler::OnIPv4Received(common::uint32_t srcIP_BE, common::uint32_t dstIP_BE, common::uint8_t* ipv4Payload,
                                    common::uint32_t size) {
    return false;
}
// Default behavior: Discard the message and don't send it back
void IPv4Handler::Send(common::uint32_t dstIP_BE, common::uint8_t* ipv4Payload,
                                                    common::uint32_t size) {
 // When we do want to send something, pass it to the backend
 backend->Send(dstIP_BE, ip_protocol, ipv4Payload, size);
}


IPv4Provider::IPv4Provider(EthernetFrameProvider* etherBackend, 
            net::AddressResolutionProtocol* arp,
            common::uint32_t gatewayIP, common::uint32_t subnet_mask) 
: EthernetFrameHandler(etherBackend, 0x800) // EtherType 0x800 = ipv4
{
    for(int i = 0; i < 255; i++) {
        handlers[i] = 0;
    }
    this->arp = arp;
    this->gatewayIP = gatewayIP;
    this->subnet_mask = subnet_mask;
}
IPv4Provider::~IPv4Provider() {
}

bool IPv4Provider::OnEthernetFrameReceived(uint8_t* etherFramePayload, uint32_t size) {
    if(size < sizeof(IPv4Header))
        return false;
    IPv4Header* header = (IPv4Header*)etherFramePayload;
    bool sendBack = false;
    if(header->dstIP == backend->GetIPAddress()) {
        int length = header->headerLength;
        if(length > size) {
            length = size;
        }
        
        if(handlers[header->protocol] != 0) {
            // Take only the data inside the frame
            sendBack = handlers[header->protocol]->OnIPv4Received(
                header->srcIP, header->dstIP,
                /* Size of IPv4 payload can be variable due to the optional
                 data, so instead of adding size of the header we add the
                 header length. Multiplied by 4 bc it's in 32 bit steps
                */
                etherFramePayload + 4*header->headerLength,
                /*
                size - 4*header->headerLength);
                Here, size is a problematic value.
                Because: This passes the complete ether frame to the next
                level. But IPv4Header can be shorter than the frame and data
                behind that can be garbage. If you do printf, you won't see
                because stuff behind it is all 0's (nullbytes/garbage). 
                IP message is inside an ethernet frame so it must be shorter
                than ethernet frame. But heartbit attack works by sending a
                ping with a fake length (too large) which caused victim to send
                back not only the ping, but a lot of data behind it which could
                include passwords etc. since it is inside the network code so
                it is probable that you catch something that's sent over the
                network like passwords on HTTP. Because of that, you shouldn't
                use the total length of the header. Instead, use the minimum
                of the lengths the header tells you it had.
                */
                length - 4*header->headerLength);
        }
        // Removed the last 4bytes of C2C checksum in the net driver already
    }
    // If the handler wants to send the data back
    if(sendBack) {
        // Swap the src and dst ip
        uint32_t temp = header->dstIP;
        header->dstIP = header->srcIP;
        header->srcIP = temp;
        // reset the TTL
        header->timeToLive = 0x40; // 64 hops
        header->checksum = 0;      // set the checksum to 0 before calculating
        header->checksum = Checksum((uint16_t*)header, 4*header->headerLength); 
    }

    return sendBack;
}
void IPv4Provider::Send(common::uint32_t dstIP_BE, common::uint8_t protocol, 
                        common::uint8_t* data, common::uint32_t size) {
    uint8_t* buffer = (uint8_t*) MemoryManager::activeMemoryManager->malloc(sizeof(IPv4Header) + size);
    IPv4Header* newHeader = (IPv4Header*) buffer;
    newHeader->version = 4;
    newHeader->headerLength = sizeof(IPv4Header)/4;
    newHeader->tos = 0;
    newHeader->totalLength = size + sizeof(IPv4Header);
    // Convert to big endian
    newHeader->totalLength =  ((newHeader->totalLength & 0xFF00) >> 8)
                            | ((newHeader->totalLength & 0x00FF) << 8); 
    newHeader->ident = 0x0100;  // set id to something random
    newHeader->flags_offset = 0x0040; // this message is not fragmented
    newHeader->timeToLive = 0x40;
    newHeader->protocol = protocol;

    newHeader->dstIP = dstIP_BE;
    newHeader->srcIP = backend->GetIPAddress();

    newHeader->checksum = 0;    // initialize w 0 bc checksum adds this value
    newHeader->checksum = Checksum((uint16_t*)newHeader, sizeof(IPv4Header));

    // Copy the data to send to the new buffer w/ the new header
    uint8_t* dataBuffer = buffer + sizeof(IPv4Header);
    
    for(int i = 0; i < size; i++) {
        dataBuffer[i] = data[i];
    }
   
    /* This is where the subnet mask and default gateway come into play. By 
        default we'll send the data to the destination. So the route is the
        resolved IP for the MAC from the ARP. If the target IP is not in our
        subnet or network, we don't talk to the recepient directly. So, we
        leave this to the gateway.
     */
    uint32_t route = dstIP_BE;  
    // Checking if the recepient is in the same LAN as ourselves
    // You have a small LAN and have a gateway machine that talks to the
    // network for you. If the recepient is not in the same LAN as you,
    // you don't resolve the recepient's ip, instead you resolve the default
    // gateway's IP and send that data to the gateway
    if((dstIP_BE & subnet_mask) != (newHeader->srcIP & subnet_mask))
        route = gatewayIP;
    // EthernetFrameHandler
    printf("\nPassed data [L3]: ");
    for(int i = 0; i < size; i++) {
        printfHex(data[i]); 
        printf(" ");
    }
    printf("\nSending [L3]: ");
    for(int i = 0; i < size; i++) {
        printfHex(buffer[i]); 
        printf(" ");
    }
    backend->Send(arp->Resolve(route), this->etherType_BE, buffer, sizeof(IPv4Header) + size);
    MemoryManager::activeMemoryManager->free(buffer);
}
// There are protocols that allow you to set the checksum to 0 and if so, it
// won't check the checksum. But IPv4 is more strict on this so we need to
// set the checksum
uint16_t IPv4Provider::Checksum(common::uint16_t* data, common::uint32_t lengthInBytes) {
    uint32_t temp = 0;
    for(int i = 0; i < lengthInBytes/2; i++) {
        temp += ((data[i] & 0xFF00) >> 8) | ((data[i] & 0x00FF) << 8);
    }
    if(lengthInBytes % 2) {
        // If it's an odd number, cast this to a 1 byte array and take the last
        // one of the values and shift it to the left making it big endian
        temp += ((uint16_t)((char*)data)[lengthInBytes-1]) << 8;
    }
    // If we have an overflow when we sum this up, we have to add the bits that
    // have overflown back into 16 bits. We have a 32 bit temp value. If we have
    // an overflow, that's the case there's something above the last 16 bits
    while(temp & 0xFFFF0000) {
        // So we take the overflown bits and add them to the last ones and we
        // do this until the first 16 bits are all 0.
        temp = (temp & 0xFFFF) + (temp >> 16);
    }
    // temp must be negated in return
    return ((~temp & 0xFF00) >> 8) | ((~temp & 0x00FF) << 8);
}
