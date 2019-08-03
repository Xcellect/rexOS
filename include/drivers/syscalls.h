#ifndef __REXOS__SYSCALLS_H
#define __REXOS__SYSCALLS_H
#include <hardwarecomm/interrupts.h>
#include <common/types.h>
#include <multitasking.h>
namespace rexos {
    class SyscallHandler : public hardwarecomm::InterruptHandler {
        public:
            SyscallHandler(hardwarecomm::InterruptManager* intManager, 
                            common::uint8_t intNumber);
            ~SyscallHandler();
            common::uint32_t HandleInterrupt(common::uint32_t esp);
    };


}

#endif