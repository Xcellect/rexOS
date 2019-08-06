#ifndef __REXOS__NET__ARP_H
#define __REXOS__NET__ARP_H
#include <common/types.h>
#include <net/ethframe.h>

namespace rexos {
    namespace net {
        struct ARPMessage {
            common::uint16_t HWType;
            common::uint16_t protocol;
            common::uint8_t HWAddressSize;
            common::uint8_t IPAddressSize;
            common::uint16_t command;
            
            common::uint64_t srcMAC : 48;
            common::uint32_t srcIP;
            common::uint64_t dstMAC : 48;
            common::uint32_t dstIP;
        } __attribute__((packed));

        class AddressResolutionProtocol : public EthernetFrameHandler {
                common::uint32_t IPcache[128];
                common::uint64_t MACcache[128];
                int numCacheEntries;
            public:
                AddressResolutionProtocol(EthernetFrameProvider* backend);
                ~AddressResolutionProtocol();
                bool OnEthernetFrameReceived(common::uint8_t* ethFramePayload, common::uint32_t size);
                // Send a request to get the MAC address of the passed IP
                // address. When we get the response this is going to the cache
                // This can be retrieved using GetMACFromCache
                void RequestMACAddress(common::uint32_t IP_BE);
                common::uint64_t GetMACFromCache(common::uint32_t IP_BE);
                // Sends a request, waits for an answer, and returns the MAC
                common::uint64_t Resolve(common::uint32_t IP_BE);

        };
    }
}
#endif