#include "mouse.h"

MouseDriver::MouseDriver(InterruptManager* manager)
: InterruptHandler(0x2C, manager),
dataport(0x60),
commandport(0x64) {     
    // Interrupt handler works on 0x2C but works on the same data & command port as kb
    
    // initializing values
    offset = 0;
    buttons = 0;
    // initializing cursor pixels at the center
    uint16_t* VideoMemory = (uint16_t*) 0xb8000;
    VideoMemory[80*12+40] = (VideoMemory[80*12+40] & 0xF000) >> 4 
                        | (VideoMemory[80*12+40] & 0x0F00) << 4 // now shift the low 4 bits to high
                        | (VideoMemory[80*12+40] & 0x00FF); // last 8 bits stay the same




    commandport.Write(0xA8);    // Activate mouse command port to get the curr state of pic
    commandport.Write(0x20);    // Get current state
    uint8_t status = dataport.Read() | 2; // Set the second bit to true
    commandport.Write(0x60); // set curr state
    dataport.Write(status); // show it on screen

    commandport.Write(0xD4);
    dataport.Write(0xF4); // Write it back
    dataport.Read();
}
MouseDriver::~MouseDriver() {
}

//void printf(char*);

uint32_t MouseDriver::HandleInterrupt(uint32_t esp) {
    uint8_t status = commandport.Read();
    // there is data to read only if the 6th bit of status is 1
    if(!(status & 0x20))
        return esp;

    // for some reason declaring this in the header makes the loader file lose its mind
    static int8_t x = 40, y = 12; // initializing x, y at the center of the screen

    // read that data into the buffer at the current offset
    buffer[offset] = dataport.Read();
    offset = (offset + 1) % 3; // then we move the offset
    // if the transmission is complete, if the 3rd byte is complete
    // the offset is 0 now

    // buffer[1] is x axis, buffer[2] is y axis

    // add this to the current pos to get a new curr pos
    if(offset == 0) {
        if(buffer[1] != 0 || buffer[2] != 0) {
        uint16_t* VideoMemory = (uint16_t*) 0xb8000;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xF000) >> 4 
                               | (VideoMemory[80*y+x] & 0x0F00) << 4 // now shift the low 4 bits to high
                               | (VideoMemory[80*y+x] & 0x00FF); // last 8 bits stay the same


        x += buffer[1];

        if(x<0) x = 0;
        if(x>=80) x = 79;

        y -= buffer[2];
        if(y<0) y = 0;
        if(y>=25) y = 24;

        // after we moved the cursor
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xF000) /* take the high 4 bits */ 
                                >> 4 /* shift 4 bits right so they're the new low bits */
                               | (VideoMemory[80*y+x] & 0x0F00) << 4 // now shift the low 4 bits to high
                               | (VideoMemory[80*y+x] & 0x00FF); // last 8 bits stay the same
        }
        // check if the button has been pressed. buffer stores the old state of the buttons.
        // compare the old state with the new one
        for(uint8_t i = 0; i < 3; i++) {
            // the bitshift produces 1 byte with 1 at ith position
            // 0b000100 <-- if i = 2 
            // finally compare that with the content of buffer 0 (curr state)
            if(buffer[0] & (0x01 << i) != buttons & (0x01 << i)) {
                uint16_t* VideoMemory = (uint16_t*) 0xb8000;
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xF000) /* take the high 4 bits */ 
                                >> 4 /* shift 4 bits right so they're the new low bits */
                               | (VideoMemory[80*y+x] & 0x0F00) << 4 // now shift the low 4 bits to high
                               | (VideoMemory[80*y+x] & 0x00FF); // last 8 bits stay the same
            }
        }
        buttons = buffer[0]; // copy buffer 0 when done
    }
    return esp;         // also forgot to return the esp which crashed the processor
}