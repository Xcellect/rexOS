#ifndef __REXOS__NET__TCP_H
#define __REXOS__NET__TCP_H
#include <common/types.h>
#include <net/ipv4.h>
namespace rexos {
    namespace net {

        // An enumeration for the states for the sockets. A socket will be in
        // one of the following states
        enum TCPSocketState {
            // 3-way handshake
            CLOSED,
            LISTEN,
            SYN_SENT,
            SYN_RECEIVED,
            ESTABLISHED,
            // Active CLOSE
            FIN_WAIT_1,
            FIN_WAIT_2,
            CLOSING,
            TIME_WAIT,
            // Passive CLOSE
            CLOSE_WAIT,
            // LAST_ACK
        };

        enum TCPSocketFlags {
            FIN = 1,
            SYN = 2,
            RST = 4,
            PSH = 8,
            ACK = 16,
            URG = 32,
            ECE = 64,
            CWR = 128,
            NS = 256
        };

        struct TCPHeader {
            common::uint16_t srcPort;
            common::uint16_t dstPort;
            common::uint32_t sequenceNum;
            common::uint32_t acknowledgementNum;
            
            
            common::uint8_t reserved : 4;
            common::uint8_t headerSize32 : 4;
            common::uint8_t flags;

            /* instead of the following, the bit from flags goes to reserved 
            As a result, reserved and headerSize32 are switched due to BE
            common::uint8_t reserved : 3;
            common::uint16_t flags : 9;
            */
            common::uint16_t windowSize;
            common::uint16_t checksum;
            common::uint16_t urgentPtr;

            common::uint32_t options;
        }__attribute__((packed));

        struct TCPPseudoHeader {
            common::uint32_t srcIP;
            common::uint32_t dstIP;
            
            common::uint16_t protocolNum;
            common::uint16_t totalLength;
        }__attribute__((packed));

        class TCPSocket;
        class TCPProvider;
        
        // Socket passes the data to the TCPHandler. You would derive your
        // applications from TCPHandler and connect them to the TCPProvider using
        // TCPSocket
        class TCPHandler {
            public:
                TCPHandler();
                ~TCPHandler();
                virtual bool HandleTCPMessage(TCPSocket* socket, 
                                common::uint8_t* data, common::uint16_t size);
        };
        // When an application wants to connect to some other machine,
        // the TCPProvider object will create something called a Socket. This will
        // maintain information about current connection
        class TCPSocket {
            friend class TCPProvider;
            protected:    
                common::uint16_t localPort;
                common::uint32_t localIP;
                common::uint16_t remotePort;
                common::uint32_t remoteIP;

                common::uint32_t acknowledgementNum;
                // The socket needs to know which number to send next as the
                // sequence number
                common::uint32_t sequenceNumber;
                TCPProvider* backend;
                TCPHandler* handler;
                
                TCPSocketState state;
            public:
                TCPSocket(TCPProvider* backend);
                ~TCPSocket();
                virtual bool HandleTCPMessage(common::uint8_t* data, 
                                            common::uint16_t size);
                virtual void Send(common::uint8_t* data, 
                                            common::uint16_t size);
                virtual void Disconnect();
        };
        // TCPProvider will get an IPv4 packet. TCPProvider object will pass the message to the
        // TCPSocket and TCPSocket will pass it to the handler
        class TCPProvider : IPv4Handler {
            // TCP will have an array of TCP sockets
            protected:
                TCPSocket* sockets[65535];
                common::uint16_t numSockets;
                common::uint16_t freeport;
            public:
                TCPProvider(IPv4Provider* backend);
                ~TCPProvider();
                virtual bool OnIPv4Received(common::uint32_t srcIP_BE, 
                        common::uint32_t dstIP_BE, common::uint8_t* ipv4Payload,
                                common::uint32_t size);
                // We'll have a function that returns TCPSocket for the passed
                // ip and port
                virtual TCPSocket* Connect(common::uint32_t ip, 
                                            common::uint16_t port);
                virtual void Disconnect(TCPSocket* socket);
                // Default flag is set to 0. A regular packet w data should
                // have its flag set to 0
                virtual void Send(TCPSocket* socket, common::uint8_t* data, 
                                    common::uint16_t size,
                                        common::uint16_t flags = 0);
                // Sets the handler of the passed socket to the passed handler
                virtual TCPSocket* Listen(common::uint16_t port);
                virtual void Bind(TCPSocket* socket, TCPHandler* handler);
        };
    }
}

#endif