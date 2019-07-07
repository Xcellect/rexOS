#ifndef __MOUSE_H
#define __MOUSE_H

#include "types.h"
#include "interrupts.h"
#include "gdt.h"
#include "port.h"

// Mouse driver is similar to that of keyboard's
class MouseDriver : public InterruptHandler {
    Port8Bit dataport;
    Port8Bit commandport;
    
    // 3 different bytes for mouse offset. Offset starting point
    // is not definite. If it doesn't work, play around w the initial
    // value of the offset (first byte)
    uint8_t buffer[3];
    uint8_t offset;
    uint8_t buttons;    // last state of the buttons
    // transmission will give the current state. this will compare
    // current state with the previous one and update that last state

public:
    MouseDriver(InterruptManager* manager);
    ~MouseDriver();
    virtual uint32_t HandleInterrupt(uint32_t esp);
};



#endif