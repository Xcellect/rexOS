#include <drivers/vga.h>
using namespace rexos::drivers;
using namespace rexos::common;


VideoGraphicsArray::VideoGraphicsArray() :
    // (V.10.) Initializing the ports in the constructor
    miscPort(0x3c2),
    CRTControllerIndexPort(0x3d4),
    CRTControllerDataPort(0x3d5),
    sequenceIndexPort(0x3c4),
    sequenceDataPort(0x3c5),
    graphicsControllerIndexPort(0x3ce),
    graphicsControllerDataPort(0x3cf),
    attributeControllerIndexPort(0x3c0),
    attributeControllerWritePort(0x3c0),
    attributeControllerReadPort(0x3c1),
    attributeControllerResetPort(0x3da) {



}

VideoGraphicsArray::~VideoGraphicsArray() {

}
// (V.14.) Sends initialization code to corresponding ports
void VideoGraphicsArray::WriteRegisters(uint8_t *registers) {
    // Increment ptr after each write
    // MISC
    miscPort.Write(*(registers++));
    
    // SEQ
    for(uint8_t i = 0; i<5; i++) {
        sequenceIndexPort.Write(i);
        sequenceDataPort.Write(*(registers++));
    }

    // CRTC
    CRTControllerIndexPort.Write(0x03); // set index 3
    // Read the old value and set its first bit to 1 (bitwise OR with 0x80)
    // and write it back to index 3
    CRTControllerDataPort.Write(CRTControllerDataPort.Read() | 0x80); 
    CRTControllerIndexPort.Write(0x11); // set index 11
    // For the index 17, we set the first bit to 0 like this
    CRTControllerDataPort.Write(CRTControllerDataPort.Read() & ~0x80);
    // Writing the [read data] back to [CRTC] setting the first bit to 0
    // This would overwrite these data again in the following loop
    // This would lock the controller again if we got wrong data in the passed
    // registers
    
    // Doing the same to the passed register values to make sure we don't
    // overwrite this in the loop 
    registers[0x03] = registers[0x03] | 0x80;
    registers[0x11] = registers[0x11] | ~0x80;

    for(uint8_t i = 0; i<25; i++) {
        CRTControllerIndexPort.Write(i);    // Where to write
        CRTControllerDataPort.Write(*(registers++));    // What to write
    }

    // GC
    for(uint8_t i = 0; i<9; i++) {
        graphicsControllerIndexPort.Write(i);
        graphicsControllerDataPort.Write(*(registers++));
    }
    // ATTRC
    for(uint8_t i = 0; i<21; i++) {
        attributeControllerResetPort.Read();
        attributeControllerIndexPort.Write(i);
        // Write to the Write port of ATTRC but reset first
        attributeControllerWritePort.Write(*(registers++));
    }
    // Reset the ATTRC again
    attributeControllerResetPort.Read();
    attributeControllerIndexPort.Write(0x20);
}


// (V.11.) Implementing 320x200 pixel w/ 8bit color depth
bool VideoGraphicsArray::SupportMode(uint32_t width, uint32_t height, uint32_t colordepth) {
    return width == 320 && height == 200 && colordepth == 8;
}


// (V.12.) A function to set the mode
// Taken from https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
bool VideoGraphicsArray::SetMode(uint32_t width, uint32_t height, uint32_t colordepth) {
    // (V.14.) If the mode is not supported return false
    if(!SupportMode(width, height, colordepth))
        return false;
    // Make sure this is 320x200x256, not fucking modex
    unsigned char g_320x200x256[] =
    {
        /* MISC */
            0x63,
        /* SEQ */
            0x03, 0x01, 0x0F, 0x00, 0x0E,
        /* CRTC */
            0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
            0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
            0xFF,
        /* GC */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
            0xFF,
        /* AC */
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
            0x41, 0x00, 0x0F, 0x00, 0x00
    };
    // (V.13.) Pass this to the WriteRegisters
    WriteRegisters(g_320x200x256);
    return true;
}




uint8_t* VideoGraphicsArray::GetFrameBufferSegment() {
    graphicsControllerIndexPort.Write(0x06);
    // We're interested in bits 3 and 4 so we we remove the last 2 bits and
    // do a bitwise AND to take last 2 bits and discard the rest
    uint8_t segementNumber = (graphicsControllerDataPort.Read() >> 2) & 0x03;
    switch(segementNumber) {
        default:
        case 0: return (uint8_t*)0x00000;    // Strange to put just 0's
        case 1: return (uint8_t*)0xA0000;
        case 2: return (uint8_t*)0xB0000;
        case 3: return (uint8_t*)0xB8000;   // Familiar from text mode
    }
}

void VideoGraphicsArray::PutPixel(int32_t x, int32_t y,
                      int8_t colorIndex) {
    if(x< 0 || 320 <=x
        || y < 0 || 200 <=y)
        return;
    // Get the frame buffer segment for the offset to set pixel at
    uint8_t* pixelAddress = GetFrameBufferSegment() +320*y + x;
    *pixelAddress = colorIndex; 
}

uint8_t VideoGraphicsArray::GetColorIndex(uint8_t r, uint8_t g, uint8_t b) {
    // VirtualBox has the following as a default setting
    // Probably standardized since it works on actual HW
    if(r == 0x00 && g == 0x00 && b == 0x00) return 0x00; // black
    if(r == 0x00 && g == 0x00 && b == 0xA8) return 0x01; // blue
    if(r == 0x00 && g == 0xA8 && b == 0x00) return 0x02; // green
    if(r == 0xA8 && g == 0x00 && b == 0x00) return 0x04; // red
    if(r == 0xFF && g == 0xFF && b == 0xFF) return 0x3F; // white
    return 0x00;
}

void VideoGraphicsArray::PutPixel(int32_t x, int32_t y, int8_t r, int8_t g, int8_t b) {
    PutPixel(x, y, GetColorIndex(r, g, b));               
}

void VideoGraphicsArray::FillRectangle(uint32_t x, uint32_t y, uint32_t w, 
                                    uint32_t h, uint8_t r, uint8_t g, uint8_t b) {
            for(uint32_t Y = y; Y < y+h; Y++) {
                for(uint32_t X = x; X < x+w; X++) {
                    PutPixel(X, Y, r, g, b);
        }
    }
}

