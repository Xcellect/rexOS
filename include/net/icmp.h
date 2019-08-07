#ifndef __REXOS__NET__ICMP_H
#define __REXOS__NET__ICMP_H
#include <net/ipv4.h>
#include <common/types.h>

namespace rexos {
    namespace net {
        struct ICMPHeader {
            common::uint8_t type;
            common::uint8_t code;
            common::uint16_t checksum;
            common::uint32_t data;
        } __attribute__ ((packed));
        
        class ICMP : IPv4Handler {
            public:
                ICMP(IPv4Provider* backend);
                ~ICMP();
                // Override the following function from IPv4Handler
                bool OnIPv4Received(common::uint32_t srcIP_BE, 
                    common::uint32_t dstIP_BE, common::uint8_t* ipv4Payload,
                                                    common::uint32_t size);
                // Take an IP that we want to ping in big endian
                void RequestEchoReply(common::uint32_t ip_be);
        };
    }
}


#endif