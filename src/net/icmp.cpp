#include <net/icmp.h>
using namespace rexos;
using namespace rexos::common;
using namespace rexos::net;


ICMP::ICMP(IPv4Provider* backend) 
: IPv4Handler(backend, 0x01)
{
}
ICMP::~ICMP() {
}
void printf(char*);
void printfHex(uint8_t);
bool ICMP::OnIPv4Received(common::uint32_t srcIP_BE, 
                    common::uint32_t dstIP_BE, common::uint8_t* ipv4Payload,
                                                    common::uint32_t size) {
    if(size < sizeof(ICMPHeader))
        return false;
    ICMPHeader* msg = (ICMPHeader*) ipv4Payload;
    switch (msg->type)
    {
    // 0: response    
    case 0:
        printf("\nPING RESPONSE FROM: ");
        printfHex(srcIP_BE & 0xFF);
        printf("."); printfHex((srcIP_BE >> 8) & 0xFF);
        printf("."); printfHex((srcIP_BE >> 16) & 0xFF);
        printf("."); printfHex((srcIP_BE >> 24) & 0xFF);
        printf("\n");
        break;
    // 8: ping request
    case 8:
        msg->type = 0;
        msg->checksum = 0;
        msg->checksum = IPv4Provider::Checksum((uint16_t*)&msg,
                        sizeof(IPv4Header));
        return true;
    }
    return false;
}
// If we want to request ping, we instantiate a header with type 0
void ICMP::RequestEchoReply(common::uint32_t ip_be) {
    ICMPHeader header;
    header.type = 8;    // We're being pinged
    header.code = 0;
    header.data = 0x3713;   // we're so 1337 we're making our own network stack
    header.checksum = 0;
    header.checksum = IPv4Provider::Checksum((uint16_t*)&header,
                        sizeof(ICMPHeader));
    // Sending using IPv4 [L3]
    IPv4Handler::Send(ip_be, (uint8_t*)&header, sizeof(ICMPHeader));
}
