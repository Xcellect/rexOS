#include <net/udp.h>
#include <common/types.h>
using namespace rexos;
using namespace rexos::net;
using namespace rexos::common;
void printf(char*);
void printfHex(uint8_t);
UDPHandler::UDPHandler() {}
UDPHandler::~UDPHandler() {}
void UDPHandler::HandleUDPMessage(UDPSocket* socket, 
                                uint8_t* data, uint16_t size) {
}
UDPSocket::UDPSocket(UDPProvider* backend) {
    this->backend = backend;
    handler = 0;    // No handler at initialization
    listening = false;
}
/* UDPSocket gets data from the application and passes it to the backend.
    It also gets data from the backend and passes it to the application
*/
UDPSocket::~UDPSocket() {}
// When UDPSocket gets a message it passes the message to the UDPHandler
void UDPSocket::HandleUDPMessage(uint8_t* data, uint16_t size) {
    if(handler != 0)
        handler->HandleUDPMessage(this,data,size);
}
void UDPSocket::Send(uint8_t* data, uint16_t size) {
    backend->Send(this,data,size);
}
// UDPSocket will always call the corresponding Disconnect function from the
// backend (UDPProvider)
void UDPSocket::Disconnect() {
    backend->Disconnect(this);
}
// IPv4 is supposed to pass it the data with protocol 17 (0x11)
UDPProvider::UDPProvider(IPv4Provider* backend) 
: IPv4Handler(backend, 0x11) // UDP is 0x11
{
    for(int i = 0; i<65535; i++) {
        this->sockets[i] = 0;
    }
    numSockets = 0;
    freeport = 1024; // First 1024 ports are reserved
}
UDPProvider::~UDPProvider() {}
bool UDPProvider::OnIPv4Received(uint32_t srcIP_BE, 
                        uint32_t dstIP_BE, uint8_t* ipv4Payload,
                        uint32_t size) {
    /*
    printf("\nRECEIVING [Layer 3]: ");
    for(int i = 0; i < (size > 64 ? 64 : size); i++) {
            // Print what we received
            printfHex(ipv4Payload[i]);
            printf(" ");
    }
    */
    // When we receive data, as usual cast it to this protocol's header struct
    // so the information becomes available
    if(size < sizeof(UDPHeader)) {
        return false;
    }
    // UDP is not supposed to change the data and send it back. It's not
    // supposed to see the data as individual packets, but as a stream
    // It's also not supposed to know where the gaps are between packets
    // Therefore, we always return false in this function
    UDPHeader* msg = (UDPHeader*) ipv4Payload;
    // You can have multiple connections (different IP addresses) on the 
    // same port
    // Iterate over the sockets to check if the current socket is the socket
    // that is supposed to get the message
    UDPSocket* socket = 0;
    for(uint16_t i = 0; i < numSockets && socket == 0; i++) {
        if(sockets[i]->localIP == dstIP_BE &&
            sockets[i]->localPort == msg->dstPort &&
            sockets[i]->listening) {
            // If so, that socket is selected
            socket = sockets[i];
            // Not listening anymore because now remote host is connected
            socket->listening = false;
            socket->remotePort = msg->srcPort;
            socket->remoteIP = srcIP_BE;
        }
        else if (sockets[i]->remoteIP == srcIP_BE && 
            sockets[i]->localIP == dstIP_BE &&
            sockets[i]->remotePort == msg->srcPort && 
            sockets[i]->localPort == msg->dstPort) {
            socket = sockets[i];
        }
    }
    // If we have found the socket, only send the IPv4 payload to the handler
    // of this socket if there is one
    if(socket != 0) {
        socket->HandleUDPMessage(ipv4Payload + sizeof(UDPHeader),
                                size - sizeof(UDPHeader));
    }

    return false;
}

UDPSocket* UDPProvider::Connect(uint32_t IP_BE, uint16_t port) {
    UDPSocket* new_socket = (UDPSocket*)MemoryManager::activeMemoryManager->malloc(sizeof(UDPSocket));
    if(new_socket != 0) {
        new (new_socket)UDPSocket(this);
        new_socket->remotePort = port; 
        new_socket->remoteIP = IP_BE;
        // Interesting thing is (same as TCP) the remote port and the local
        // port do not have to match. So you could pass a port on the remote
        // machine but the local machine here can connect using any port it
        // wants
        new_socket->localPort = freeport++; // Use any port after the reserved ports
        // Could be set to 127.0.0.1 and the socket would accept only local
        // connections
        new_socket->localIP = backend->GetIPAddress();
        new_socket->remotePort = ((new_socket->remotePort & 0xFF00) >> 8) 
                            | ((new_socket->remotePort & 0x00FF) << 8);
        new_socket->localPort = ((new_socket->localPort & 0xFF00) >> 8) 
                            | ((new_socket->localPort & 0x00FF) << 8);                            

        sockets[numSockets++] = new_socket;
    }
    return new_socket;
}
// If we listen on a port, it means we created a socket with listening mode
// We don't set the remote port and IP right away, but in OnIPv4Received 
// function when the first packet tries to reach here
UDPSocket* UDPProvider::Listen(uint16_t port) {
    UDPSocket* new_socket = (UDPSocket*)MemoryManager::activeMemoryManager->malloc(sizeof(UDPSocket));
    if(new_socket != 0) {
        new (new_socket)UDPSocket(this);
        // new_socket->remotePort = port; 
        // new_socket->remoteIP = IP_BE;
        new_socket->listening = true;
        new_socket->localPort = port;
        new_socket->localIP = backend->GetIPAddress();
        new_socket->localPort = ((new_socket->localPort & 0xFF00) >> 8) 
                            | ((new_socket->localPort & 0x00FF) << 8);                            
        sockets[numSockets++] = new_socket;
    }
    return new_socket;
}

bool UDPProvider::Disconnect(UDPSocket* socket) {
    // Remove the socket from the list
    for(uint16_t i = 0; i < numSockets && socket == 0; i++) {
        // If found, overwrite with the last socket in the list
        if(sockets[i] == socket) {
            // If there is only 1 socket, it overwrites with itself since the value
            // of numSocket pre decrements which means i will be bigger in the next
            // iteration and exit
            sockets[i] = sockets[--numSockets]; 
            MemoryManager::activeMemoryManager->free(socket);
            break;
        }
    }
}
void UDPProvider::Send(UDPSocket* socket, uint8_t* data, 
                                            uint16_t size) {
    // Length of the data we want to send + the UDP header
    uint16_t totalLength = size + sizeof(UDPHeader);
    // Allocating RAM for the data to send
    uint8_t* buffer = (uint8_t*) MemoryManager::activeMemoryManager->malloc(totalLength);
    // Message starts right after the header
    uint8_t* bufferMessage = buffer + sizeof(UDPHeader);
    // Assign the buffer's address to the new message
    UDPHeader* msg = (UDPHeader*) buffer;
    msg->srcPort = socket->localPort;
    msg->dstPort = socket->remotePort;
    msg->length = ((totalLength & 0x00FF) << 8) | ((totalLength & 0xFF00) >> 8);
    // Copy data to the buffer
    for(int i = 0; i < size; i++) {
        bufferMessage[i] = data[i];
    }
    /* 
    printf("\nPassed data [Layer 4]: ");
    for(int i = 0; i < size; i++) {
        printfHex(data[i]); 
        printf(" ");
    }
    printf("\nSending [Layer 4]: ");
    for(int i = 0; i < size; i++) {
        printfHex(buffer[i]); 
        printf(" ");
    }
    */
    // Computation for layer 4 checksum involves pseudo header which constains
    // some data from IPv4 header which absolutely doesn't belong here. Quite 
    // crazy. Good news: we can set it to 0 here because UDP does not require
    // this. This deactivates the error checking
    msg->checksum = 0;
    IPv4Handler::Send(socket->remoteIP, buffer, totalLength);
    MemoryManager::activeMemoryManager->free(buffer);
}

void UDPProvider::Bind(UDPSocket* socket, UDPHandler* handler) {
    socket->handler = handler;
}