#include <net/arp.h>
using namespace rexos;
using namespace rexos::common;
using namespace rexos::net;
using namespace rexos::drivers;

void printf(char*);
void printfHex(uint8_t);

AddressResolutionProtocol::AddressResolutionProtocol(EthernetFrameProvider* backend) 
: EthernetFrameHandler(backend, 0x806)
{
    numCacheEntries = 0;
}
AddressResolutionProtocol::~AddressResolutionProtocol() {
}
bool AddressResolutionProtocol::OnEthernetFrameReceived(uint8_t* etherFramePayload, uint32_t size) {
    if(size < sizeof(AddressResolutionProtocol))
        return false;
    // Cast the ethFramePayload to arp struct (data is aligned to this format)
    ARPMessage* arp = (ARPMessage*) etherFramePayload;
    if(arp->HWType == 0x0100) {         // ethernet message
        if(arp->protocol == 0x0008
            && arp->HWAddressSize == 6
            && arp->IPAddressSize == 4
            // If it's not for us, we could look into the cache for the requested
            // IP address and reply back with its MAC and/or forward the
            // request to some other machine (behavior of a gateway)
            && arp->dstIP == backend->GetIPAddress()) {
                switch (arp->command)
                {
                case 0x0100:    // Asked for our MAC address
                   arp->command = 0x0200;   // change the command to response
                   arp->dstIP = arp->srcIP;
                   arp->dstMAC = arp->srcMAC;
                   arp->srcIP = backend->GetIPAddress();
                   arp->dstMAC = backend->GetMACAddress(); 
                   return true;
                   break;
                case 0x0200:    // Got a response to our request
                   // Put the responder's MAC and IP in our cache
                   if(numCacheEntries < 128) {
                    IPcache[numCacheEntries] = arp->srcIP;
                    MACcache[numCacheEntries] = arp->srcMAC;
                    numCacheEntries++; 
                   }
                   /* 
                    Not a good idea to store an IP we haven't asked for.
                    These messages aren't reliable anyway. If we ask the
                    some server, what the MAC address of a specific IP
                    is, it might reply with its own MAC even though it's
                    not its own IP. e.g. If you ask for the MAC of google.com
                    and someone else replies with their own IP, effectively
                    you'll be talking to them every time you make a google
                    request. This is an opportunity for ARP spoofing and MitM
                    attack. But you're getting some data which you cannot
                    validate here. Even if it's a communication through SSL,
                    you need the public key of the other machine and so on.
                    So it'll stay like this for now.
                   */
                   break;
                }
        
        }
    }
    return false;
}
void AddressResolutionProtocol::RequestMACAddress(uint32_t IP_BE) {
    ARPMessage arp;
    arp.HWType = 0x0100; // ethernet
    arp.protocol = 0x0008; // IPv4
    arp.HWAddressSize = 6;
    arp.IPAddressSize = 4;
    arp.command = 0x0100;   // request

    arp.srcMAC = backend->GetMACAddress();
    arp.srcIP = backend->GetIPAddress();
    arp.dstMAC = 0xFFFFFFFFFFFF;
    arp.dstIP = IP_BE;
    uint8_t* buffer = (uint8_t*)&arp;
    /*
    printf("\nSending [Layer 3]: ");
    for(int i = 0; i < sizeof(ARPMessage); i++) {
        printfHex(buffer[i]); 
        printf(" ");
    }
    */
    // Request the unknown IP's MAC address
    this->Send(arp.dstMAC, (uint8_t*)&arp, sizeof(ARPMessage));
}
uint64_t AddressResolutionProtocol::GetMACFromCache(uint32_t IP_BE) {
    for(int i = 0; i < numCacheEntries; i++) {
        if(IPcache[i] == IP_BE) {
            return MACcache[i];
        }
    }
    return 0xFFFFFFFFFFFF;  // bcast address
}


common::uint64_t AddressResolutionProtocol::Resolve(common::uint32_t IP_BE) {
    uint64_t result = GetMACFromCache(IP_BE);
    if(result == 0xFFFFFFFFFFFF) {
        printf("[ARP: UNKNOWN REMOTE IP. REQUESTING REMOTE MAC.]");
        RequestMACAddress(IP_BE);
    }
    // possible infinite loop if the machine we requested isn't connected
    // need a timeout
    while(result == 0xFFFFFFFFFFFF) {   
        result = GetMACFromCache(IP_BE);
    }
    printf("\n[ARP: REMOTE IP RESOLVED. READY TO SEND.]");
    return result;
}
/* When we request an ICMP echo reply to some machine, it doesn't know our
MAC (& IP) address yet and so it cannot answer the ping request. Instead, it 
sends an ARP request using broadcast MAC. Event if we reply to this broadcast,
we won't get the answer to the ping. SendMACAddress responds to the ARP request
made from ICMP with our MAC and IP.
 */
// Might be a better idea to broadcast this MAC address to all machines on the
// local network
void AddressResolutionProtocol::SendMACAddress(common::uint32_t IP_BE) {
    printf("[ARP: SENDING HOST MAC TO REMOTE IP.]\n");
    ARPMessage arp;
    arp.HWType = 0x0100; // ethernet
    arp.protocol = 0x0008; // IPv4
    arp.HWAddressSize = 6;
    arp.IPAddressSize = 4;
    arp.command = 0x0200;   // response

    arp.srcMAC = backend->GetMACAddress();
    arp.srcIP = backend->GetIPAddress();
    arp.dstMAC = Resolve(IP_BE);
    arp.dstIP = IP_BE;
    uint8_t* buffer = (uint8_t*)&arp;
    /*
    printf("\nSending [Layer 3]: ");
    for(int i = 0; i < sizeof(ARPMessage); i++) {
        printfHex(buffer[i]); 
        printf(" ");
    }
    */
    // We have inherited EthernetFrameHandler's send function which will be
    // used instead of that of the backend (EthernetFrameProvider) (sends
    // the payload manually)
    
    this->Send(arp.dstMAC, (uint8_t*)&arp, sizeof(ARPMessage));
}