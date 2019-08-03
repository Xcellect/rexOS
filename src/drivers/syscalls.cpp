#include <drivers/syscalls.h>
using namespace rexos;
using namespace rexos::common;
using namespace rexos::hardwarecomm;

SyscallHandler::SyscallHandler(hardwarecomm::InterruptManager* intManager, 
                            common::uint8_t intNumber) 
// Attaching SyscallHandler to the InterruptManager
: InterruptHandler(intNumber + intManager->HWInterruptOffset(), intManager)   
{

}
SyscallHandler::~SyscallHandler() {
}


void printf(char*);

common::uint32_t SyscallHandler::HandleInterrupt(common::uint32_t esp) {
    // The esp still points to the start of the CPUState struct
    CPUState* cpu = (CPUState*) esp; // Casting it like that in the int handler
    
    // POSIX (Portable Operating System Interface)
    switch(cpu->eax) {
        case 4:
            printf((char*)cpu->ebx);
            break;
        default:
            break;            
    }

    return esp;
}