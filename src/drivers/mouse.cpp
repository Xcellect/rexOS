#include <drivers/mouse.h>
using namespace rexos::drivers;
using namespace rexos::common;
using namespace rexos::hardwarecomm;

MouseEventHandler::MouseEventHandler() {

}
void MouseEventHandler::OnActivate() {

}
void MouseEventHandler::OnMouseClick(uint8_t button) {

}
void MouseEventHandler::OnMouseUnclick(uint8_t button) {

}
void MouseEventHandler::OnMouseMove(int x, int y) {

}



MouseDriver::MouseDriver(InterruptManager* manager, MouseEventHandler* handler)
: InterruptHandler(0x2C, manager),
dataport(0x60),
commandport(0x64) {     
    this->handler = handler;
}
MouseDriver::~MouseDriver() {
}

//void printf(char*);

void MouseDriver::Activate() {
    // Interrupt handler works on 0x2C but works on the same data & command port as kb
    
    // initializing values
    offset = 0;
    buttons = 0;
    


    commandport.Write(0xA8);    // Activate mouse command port to get the curr state of pic
    commandport.Write(0x20);    // Get current state
    uint8_t status = dataport.Read() | 2; // Set the second bit to true
    commandport.Write(0x60); // set curr state
    dataport.Write(status); // show it on screen

    commandport.Write(0xD4);
    dataport.Write(0xF4); // Write it back
    dataport.Read();
}

uint32_t MouseDriver::HandleInterrupt(uint32_t esp) {
    uint8_t status = commandport.Read();
    // there is data to read only if the 6th bit of status is 1
    if(!(status & 0x20))
        return esp;
    buffer[offset] = dataport.Read();
    if(handler == 0)
        return esp;

    // for some reason declaring this in the header makes the loader file lose its mind
    //static int8_t x = 40, y = 12; // initializing x, y at the center of the screen

    // read that data into the buffer at the current offset
    
    offset = (offset + 1) % 3; // then we move the offset
    // if the transmission is complete, if the 3rd byte is complete
    // the offset is 0 now

    // buffer[1] is x axis, buffer[2] is y axis

    // add this to the current pos to get a new curr pos
    if(offset == 0) {
        if(buffer[1] != 0 || buffer[2] != 0) {
            handler->OnMouseMove(buffer[1], -buffer[2]);
            
        }
        // check if the button has been pressed. buffer stores the old state of the buttons.
        // compare the old state with the new one
        for(uint8_t i = 0; i < 3; i++) {
            // the bitshift produces 1 byte with 1 at ith position
            // 0b000100 <-- if i = 2 
            // finally compare that with the content of buffer 0 (curr state)
            if(buffer[0] & (0x01 << i) != buttons & (0x01 << i)) {
                if(buttons & (0x1<<i))
                    handler->OnMouseUnclick(i+1);
                else
                    handler->OnMouseClick(i+1);
                
            }
        }
        buttons = buffer[0]; // copy buffer 0 when done
    }
    return esp;         // also forgot to return the esp which crashed the processor
}