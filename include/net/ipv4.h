#ifndef __REXOS__NET__IPV4_H
#define __REXOS__NET__IPV4_H

#include <common/types.h>
#include <net/ethframe.h>
#include <net/arp.h>

namespace rexos {
    namespace net {
        struct IPv4Header {
            common::uint8_t headerLength : 4;   // 4 bits
            common::uint8_t version : 4;    // 4 for IPv4
            common::uint8_t tos;
            common::uint16_t totalLength;

            common::uint16_t ident;
             
            // common::uint8_t flags : 3;  // 3
            // common::uint8_t fragmentOffset : 13;
            
            common::int16_t flags_offset;           
            common::uint8_t timeToLive;
            common::uint8_t protocol;
            common::uint16_t checksum;

            common::uint32_t srcIP;
            common::uint32_t dstIP;
        } __attribute__((packed));
        class IPv4Provider;
        // Similar to EthernetFrameHandler
        class IPv4Handler {
            protected:
                IPv4Provider* backend;
                common::uint8_t ip_protocol;
            public:
                IPv4Handler(IPv4Provider* backend, common::uint8_t protocol);
                ~IPv4Handler();
                // You could have multiple connections to different IPs on
                // same port. So we need src and dst IP to distinguish
                virtual bool OnIPv4Received(common::uint32_t srcIP_BE, common::uint32_t dstIP_BE, common::uint8_t* ipv4Payload,
                                                    common::uint32_t size);
                
                void Send(common::uint32_t srcIP_BE, common::uint8_t* ipv4Payload,
                                                    common::uint32_t size);
        };

        // Derived from EthernetFrameHandler because we'll pass the
        // EthernetFrameProvider. IPv4Provider looks at the IPv4Message passed 
        // to it and passes it to the IPv4Handler
        class IPv4Provider : EthernetFrameHandler {
            friend class IPv4Handler;
            protected:
                // Only have one byte
                IPv4Handler* handlers[255];
                AddressResolutionProtocol* arp;
                common::uint32_t gatewayIP;
                common::uint32_t subnet_mask;
            public:
                // Args: 1. EthernetFrameProvider as backend (passed to the 
                // base) 2. AddressResolutionProtocol as arp so we can use
                // its resolve function to get MAC addr for an IP addr. So
                // we get the IP addr of the default gateway and subnet mask
                IPv4Provider(net::EthernetFrameProvider* backend, 
                            net::AddressResolutionProtocol* arp,
                            common::uint32_t gatewayIP, common::uint32_t subnet_mask);
                ~IPv4Provider();
                // Here we receive data (ethernet frame payload) from EthernetFrameHandler
                bool OnEthernetFrameReceived(common::uint8_t* etherFramePayload,
                                                    common::uint32_t size);
                void Send(common::uint32_t dstIP_BE, common::uint8_t protocol, 
                        common::uint8_t* data, common::uint32_t size);

                static common::uint16_t Checksum(common::uint16_t* data, common::uint32_t lengthInBytes);
                common::uint32_t GetIPAddress();
        };
    }
}
#endif
