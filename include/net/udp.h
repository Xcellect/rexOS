#ifndef __REXOS__NET__UDP_H
#define __REXOS__NET__UDP_H
#include <common/types.h>
#include <net/ipv4.h>
namespace rexos {
    namespace net {
        struct UDPHeader
        {
            common::uint16_t srcPort;
            common::uint16_t dstPort;
            common::uint16_t length;
            common::uint16_t checksum;
        }__attribute__((packed));
        class UDPSocket;
        class UDPProvider;
        
        // Socket passes the data to the UDPHandler. You would derive your
        // applications from UDPHandler and connect them to the UDPProvider using
        // UDPSocket
        class UDPHandler {
            public:
                UDPHandler();
                ~UDPHandler();
                virtual void HandleUDPMessage(UDPSocket* socket, 
                                common::uint8_t* data, common::uint16_t size);
        };
        // When an application wants to connect to some other machine,
        // the UDPProvider object will create something called a Socket. This will
        // maintain information about current connection
        class UDPSocket {
            friend class UDPProvider;
            protected:    
                common::uint16_t localPort;
                common::uint32_t localIP;
                common::uint16_t remotePort;
                common::uint32_t remoteIP;
                UDPProvider* backend;
                UDPHandler* handler;
                bool listening;
            public:
                UDPSocket(UDPProvider* backend);
                ~UDPSocket();
                virtual void HandleUDPMessage(common::uint8_t* data, 
                                            common::uint16_t size);
                virtual void Send(common::uint8_t* data, 
                                            common::uint16_t size);
                virtual void Disconnect();
        };
        // UDPProvider will get an IPv4 packet. UDPProvider object will pass the message to the
        // UDPSocket and UDPSocket will pass it to the handler
        class UDPProvider : IPv4Handler {
            // UDP will have an array of UDP sockets
            protected:
                UDPSocket* sockets[65535];
                common::uint16_t numSockets;
                common::uint16_t freeport;
            public:
                UDPProvider(IPv4Provider* backend);
                ~UDPProvider();
                virtual bool OnIPv4Received(common::uint32_t srcIP_BE, 
                        common::uint32_t dstIP_BE, common::uint8_t* ipv4Payload,
                                common::uint32_t size);
                // We'll have a function that returns UDPSocket for the passed
                // ip and port
                virtual UDPSocket* Connect(common::uint32_t ip, 
                                            common::uint16_t port);
                virtual UDPSocket* Listen(common::uint16_t port);
                virtual bool Disconnect(UDPSocket* socket);
                virtual void Send(UDPSocket* socket, common::uint8_t* data, 
                                            common::uint16_t size);
                // Sets the handler of the passed socket to the passed handler
                virtual void Bind(UDPSocket* socket, UDPHandler* handler);
        };
    }
}
#endif