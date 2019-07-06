#ifndef __REXOS__DRIVERS__MOUSE_H
#define __REXOS__DRIVERS__MOUSE_H
#include <common/types.h>
#include <hardwarecomm/interrupts.h>
#include <drivers/driver.h>
#include <hardwarecomm/port.h>
namespace rexos {
	namespace drivers {
        

        // Mouse driver is similar to that of keyboard's
        // extends Driver since mouse drivers are drivers

        class MouseEventHandler {
            public:
                MouseEventHandler();
            virtual void OnActivate();
            virtual void OnMouseClick(rexos::common::uint8_t button);
            virtual void OnMouseUnclick(rexos::common::uint8_t button);
            virtual void OnMouseMove(int x, int y);
        };


        class MouseDriver : public rexos::hardwarecomm::InterruptHandler, public Driver {
            rexos::hardwarecomm::Port8Bit dataport;
            rexos::hardwarecomm::Port8Bit commandport;
            
            // 3 different bytes for mouse offset. Offset starting point
            // is not definite. If it doesn't work, play around w the initial
            // value of the offset (first byte)
            rexos::common::uint8_t buffer[3];
            rexos::common::uint8_t offset;
            rexos::common::uint8_t buttons;    // last state of the buttons
            // transmission will give the current state. this will compare
            // current state with the previous one and update that last state
            MouseEventHandler* handler;
            
        public:
            MouseDriver(rexos::hardwarecomm::InterruptManager* manager, MouseEventHandler* handler);
            ~MouseDriver();
            virtual rexos::common::uint32_t HandleInterrupt(rexos::common::uint32_t esp);
            virtual void Activate();
        };
    }
}



#endif