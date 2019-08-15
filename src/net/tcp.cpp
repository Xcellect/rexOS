#include <net/tcp.h>
#include <common/types.h>
using namespace rexos;
using namespace rexos::net;
using namespace rexos::common;
void printf(char*);
void printfHex(uint8_t);
uint32_t bigEndian32(uint32_t);
/* TCPHandler can receive raw data from TCP socket */
TCPHandler::TCPHandler() {}
TCPHandler::~TCPHandler() {}
bool TCPHandler::HandleTCPMessage(TCPSocket* socket, 
                                uint8_t* data, uint16_t size) {
    return true;
}
TCPSocket::TCPSocket(TCPProvider* backend) {
    this->backend = backend;
    handler = 0;    // No handler at initialization
    state = CLOSED;
}
TCPSocket::~TCPSocket() {}
bool TCPSocket::HandleTCPMessage(uint8_t* data, uint16_t size) {
    if(handler != 0)
        return handler->HandleTCPMessage(this,data,size);
    return false;
}
void TCPSocket::Send(uint8_t* data, uint16_t size) {
    // In the kernel, we connect and immediately try to send the message there.
    // If the message is sent before getting the SYN | ACK and sending back the
    // final ACK, it wouldn't reach the other side. Could become an infinite 
    // loop.
    while(state != ESTABLISHED) {
    }
    backend->Send(this,data,size, PSH|ACK);
}
void TCPSocket::Disconnect() {
    backend->Disconnect(this);
}
TCPProvider::TCPProvider(IPv4Provider* backend) 
: IPv4Handler(backend, 0x06) // TCP is 0x06
{
    for(int i = 0; i<65535; i++) {
        this->sockets[i] = 0;
    }
    numSockets = 0;
    freeport = 1024; // First 1024 ports are reserved
}
TCPProvider::~TCPProvider() {}
bool TCPProvider::OnIPv4Received(uint32_t srcIP_BE, 
                        uint32_t dstIP_BE, uint8_t* ipv4Payload,
                        uint32_t size) {
   
    /* TCP header that we use here has 4 extra bytes for options which are not 
    necessarily there. The smallest legal size of a TCP header is 20 bytes so
    we'll use that
    */
    if(size < 20) {
        return false;
    }
    TCPHeader* header = (TCPHeader*) ipv4Payload;
    TCPSocket* socket = 0;
    // Same as UDP. Iterates over sockets and looks for the one that can handle
    // this message
    for(uint16_t i = 0; i < numSockets && socket == 0; i++) {
        // If the sender wants to synchronize and this is a listening socket
        if(sockets[i]->localIP == dstIP_BE &&
            sockets[i]->localPort == header->dstPort &&
            sockets[i]->state == LISTEN &&
            (((header->flags) & (SYN | ACK)) == SYN)) {
              
            socket = sockets[i];
        }
        else if (sockets[i]->remoteIP == srcIP_BE && 
            sockets[i]->localIP == dstIP_BE &&
            sockets[i]->remotePort == header->srcPort && 
            sockets[i]->localPort == header->dstPort) {
                
            socket = sockets[i];
        }
    }
    bool reset = false;
    // Before starting handling, we check the RST flag first
    if(socket != 0 && header->flags & RST) {
        socket->state = CLOSED;
    }

    if(socket != 0 && socket->state != CLOSED) {
        // printf("\nFlags: ");
        // printfHex(header->flags);
        // Only look at the SYN, ACK, and FIN messages
        switch((header->flags) & (SYN | ACK | FIN)) {
            case SYN:
                if(socket->state == LISTEN) {
                    // While listening, if we get a SYN, we'll set it to SYN_RECEIVED
                    socket->state = SYN_RECEIVED;
                    socket->remotePort = header->srcPort;
                    socket->remoteIP = srcIP_BE;
                    // Set the ack num to be sender's seq num + 1
                    socket->acknowledgementNum = 
                                bigEndian32(header->sequenceNum) + 1;
                    socket->sequenceNumber = 0xbeefcafe;
                    Send(socket, 0, 0, SYN | ACK);
                    socket->sequenceNumber++;
                    
                } else {
                    reset = true;
                }
                break;
            case SYN | ACK:
                // We should only get a SYN | ACK if we sent a SYN before
                if(socket->state == SYN_SENT) {
                    socket->state = ESTABLISHED;
                    socket->acknowledgementNum = 
                                bigEndian32(header->sequenceNum) + 1;
                    socket->sequenceNumber++;
                    Send(socket,0,0,ACK);
                } else {
                    reset = true;
                }
                break;
            /* ACK and FIN could be in different messages. Here, both are
            treated the same way. If we get a FIN or a FIN/ACK, we send an
            ACK. It's okay to include FIN in that message so in both cases
            we go to some common state of "I'm currently closing" and sending
            FIN and ACK both.
             */
            case SYN | FIN:
            case SYN | FIN | ACK:
                reset = true;
                break;
            case FIN:
            case FIN | ACK:
                if(socket->state == ESTABLISHED) {
                    socket->state = CLOSE_WAIT;
                    socket->acknowledgementNum++;
                    Send(socket, 0, 0, ACK);
                    // We're sending both FIN and ACK together so CLOSE_WAIT 
                    // shouldn't exist. Therefore, LAST_ACK is not necessary
                    Send(socket, 0, 0, FIN | ACK);
                } else if (socket->state == CLOSE_WAIT) {
                    // If we're in this stage, we got a FIN, we sent an ACK and
                    // our own FIN (FIN | ACK), so now we should get the ACK or
                    // FIN | ACK
                    socket->state = CLOSED;
                } else if (socket->state == FIN_WAIT_1 ||
                            socket->state == FIN_WAIT_2) {
                    // We send a FIN, we receive a FIN | ACK that means we have
                    // the FIN and the ACK so we should go to TIME_WAIT. This 
                    // will not be implemented here. We just send an ACK back
                    socket->state = CLOSED;
                    socket->acknowledgementNum++;
                    Send(socket,0,0,ACK);
                } else {
                    reset = true;
                }
                break;
            case ACK:
                if (socket->state == SYN_RECEIVED) {
                    // Means we were listening, we got a SYN, sent a SYN | ACK and
                    // got a final ACK
                    socket->state = ESTABLISHED;
                    return false;   // So not going any further
                } else if (socket->state == FIN_WAIT_1) {
                    // In FIN_WAIT_2, we get the FIN and the ACK in different
                    // messages
                    socket->state = FIN_WAIT_2;
                    return false;
                } else if (socket->state == CLOSE_WAIT) {
                    socket->state = CLOSED;
                    break;
                }
                // Piggy-backing: Sending your data with an ACK to the previous
                // message
                if(header->flags == ACK)
                    break;
            default:
                  
                // if the message's seq num and our socket's ack num aren't
                // same, the messages have arrived in wrong order so we cant
                // handle it yet
                if(bigEndian32(header->sequenceNum) == socket->acknowledgementNum) {
                    // The boolean returned from the handler determines if we
                    // should close the connection. We took the complement bc
                    // the handler always returns false.
                    reset = !(socket->HandleTCPMessage(ipv4Payload
                    // Look at the header size that was written there
                            + header->headerSize32*4, size - header->headerSize32*4));
                    if(!reset) {
                        
                        // If we don't reset i.e. after passing data to the 
                        // handler, if it wants to keep the connection alive,
                        // we acknowledge the data we got by adding the payload
                        // size
                        socket->acknowledgementNum += size - header->headerSize32*4;
                        Send(socket,0,0,ACK);
                    }
                } else {
                    // Packets are in wrong order
                    reset = true;
                }
        }
    }
    
    if(reset) {
        // If we're resetting socket that we already have, i.e. we have a
        // and we're receiving illegal data on it, use that socket to send RST
        if(socket != 0) {
            Send(socket, 0, 0, RST);
        }
        // Problematic case is when we don't have a socket yet where we're
        // data.
        else {
            TCPSocket sock(this);
            sock.localPort = header->dstPort;
            sock.localIP = dstIP_BE;
            sock.remotePort = header->srcPort;
            sock.remoteIP = srcIP_BE;
            sock.acknowledgementNum = bigEndian32(header->sequenceNum) + 1;
            sock.sequenceNumber = bigEndian32(header->acknowledgementNum);
            // Passing this instance of socket by its address
            Send(&sock, 0, 0, RST);
        }
        return true;
    }

    // At the end of the handling, if the socket is closed, we remove it from 
    // the list
    if(socket != 0 && socket->state == CLOSED) {
        for(uint16_t i = 0; i < numSockets && socket == 0; i++) {
            if(sockets[i] == socket) {
                sockets[i] = sockets[--numSockets]; 
                MemoryManager::activeMemoryManager->free(socket);
                break;
            }
        }
    }
    return false;
}

    /*
    Handling of removing the socket from the list is not done in the Disconnect
    function anymore but in the handler function of the IP message, so when we
    received the last acknowledge, we remove the socket from this list.
    
    */
    



/***************************************************************************** */
uint32_t bigEndian32(uint32_t value) {
    return ((value & 0xFF000000) >> 24 )
        | ((value & 0x00FF0000) >> 8)
        | ((value & 0x0000FF00) << 8)
        | ((value & 0x000000FF) << 24);
}


void TCPProvider::Send(TCPSocket* socket, uint8_t* data, 
                                            uint16_t size, uint16_t flags) {
    uint16_t totalLength = size + sizeof(TCPHeader);
    uint16_t lengthInclPsHeader = totalLength + sizeof(TCPPseudoHeader);
    
    uint8_t* buffer = (uint8_t*) MemoryManager::activeMemoryManager->malloc(lengthInclPsHeader);
    
    // Pseudo header is at the start of the buffer
    TCPPseudoHeader* psHdr = (TCPPseudoHeader*) (buffer);
    // Actual header is after the pseudo header
    TCPHeader* header = (TCPHeader*) (buffer + sizeof(TCPPseudoHeader));
    // Data comes after that
    uint8_t* bufferMessage = buffer + sizeof(TCPHeader) + sizeof(TCPPseudoHeader);
    header->headerSize32 = sizeof(TCPHeader)/4;
    header->srcPort = socket->localPort;
    header->dstPort = socket->remotePort;

    header->acknowledgementNum = bigEndian32(socket->acknowledgementNum);
    header->sequenceNum = bigEndian32(socket->sequenceNumber);
    header->reserved = 0;
    header->flags = flags;
    header->windowSize = 0xFFFF;   // Set frame to max
    header->urgentPtr = 0;
    // If we send something that contains the SYN flag, then we set this option
    header->options = ((flags & SYN) != 0) ? 0xB4050402 : 0;
    // Increment the sequence number for the next msg by the passed data size
    // but it should be set to the acknowledgement number in the ACK case in
    // the OnIPv4Received() function
    socket->sequenceNumber+=size;
    // Copy the data to the buffer
    for(int i = 0; i < size; i++) {
        bufferMessage[i] = data[i];
    }

    
    psHdr->srcIP = socket->localIP;
    psHdr->dstIP = socket->remoteIP;
    psHdr->protocolNum = 0x0600; // protocol num to send to the IPv4 (Layer 3)
    // Pseudo header length is the total length of the message with itself
    psHdr->totalLength = ((totalLength & 0x00FF) << 8) | ((totalLength & 0xFF00) >> 8); 

    header->checksum = 0;
    header->checksum = IPv4Provider::Checksum((uint16_t*)buffer, lengthInclPsHeader);

    IPv4Handler::Send(socket->remoteIP, (uint8_t*)header, totalLength);
    MemoryManager::activeMemoryManager->free(buffer);
}

TCPSocket* TCPProvider::Connect(uint32_t IP_BE, uint16_t port) {
    TCPSocket* new_socket = (TCPSocket*)MemoryManager::activeMemoryManager->malloc(sizeof(TCPSocket));
    if(new_socket != 0) {
        // SYN (Connect) request is being send by this instance
        new (new_socket)TCPSocket(this);

        new_socket->remotePort = port; 
        new_socket->remoteIP = IP_BE;
        new_socket->localPort = freeport++; // Use any port after the reserved ports
        new_socket->localIP = backend->GetIPAddress();
        
        new_socket->remotePort = ((new_socket->remotePort & 0xFF00) >> 8) 
                            | ((new_socket->remotePort & 0x00FF) << 8);
        new_socket->localPort = ((new_socket->localPort & 0xFF00) >> 8) 
                            | ((new_socket->localPort & 0x00FF) << 8);                            

        sockets[numSockets++] = new_socket;
        new_socket->state = SYN_SENT;
        // We need to store our own sequence number
        new_socket->sequenceNumber = 0xbeefcafe;    // funny hex
        // We'll send the SYN after the socket is connected to the TCP backend
        // (TCPHandler) bc otherwise, we could already receive a SYN/ACK reply
        // that we wouldn't get if we sent the SYN before setting the handler
        // correctly
        Send(new_socket, 0, 0, SYN);
    }
    return new_socket;
}
// When we disconnect, we change the states
void TCPProvider::Disconnect(TCPSocket* socket) {
    // When we actively close, we send FIN and go to FIN_WAIT_1. 
    socket->state = FIN_WAIT_1;
    Send(socket,0,0,FIN + ACK);
    socket->sequenceNumber++;
}

TCPSocket* TCPProvider::Listen(uint16_t port) {
    TCPSocket* new_socket = (TCPSocket*)MemoryManager::activeMemoryManager->malloc(sizeof(TCPSocket));
    if(new_socket != 0) {
        new (new_socket)TCPSocket(this);
        new_socket->state = LISTEN;
        new_socket->localIP = backend->GetIPAddress();
        new_socket->localPort = ((port & 0xFF00) >> 8) 
                            | ((port & 0x00FF) << 8);                            
        sockets[numSockets++] = new_socket;
    }
    return new_socket;
}

void TCPProvider::Bind(TCPSocket* socket, TCPHandler* handler) {
    socket->handler = handler;
}
