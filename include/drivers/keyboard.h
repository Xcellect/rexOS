#ifndef __REXOS__DRIVERS__KEYBOARD_H
#define __REXOS__DRIVERS__KEYBOARD_H
#include <common/types.h>
#include <hardwarecomm/interrupts.h>
#include <drivers/driver.h>
#include <hardwarecomm/port.h>
namespace rexos {
	namespace drivers {
        

        /*  
            - it's not a good idea to have kb driver 
                and print the keyset print directly 
                to screen	
            
            * this wont help if we put a graphics mode 
                in between and want to have a desktop
            * keys are in a part of memory where you dont see them 
        */

        class KeyboardEventHandler {
            public:
                KeyboardEventHandler();
                virtual void OnKeyUp(char c);
                virtual void OnKeyDown(char c);
        };

        class KeyboardDriver : public rexos::hardwarecomm::InterruptHandler, public Driver {
            rexos::hardwarecomm::Port8Bit dataport;
            rexos::hardwarecomm::Port8Bit commandport;

            KeyboardEventHandler* handler;
        public:
            KeyboardDriver(rexos::hardwarecomm::InterruptManager* manager, KeyboardEventHandler* handler);
            ~KeyboardDriver();
            virtual rexos::common::uint32_t HandleInterrupt(rexos::common::uint32_t esp);
            virtual void Activate();
        };
    }
}



#endif