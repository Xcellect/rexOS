#ifndef __REXOS__DRIVERS__VGA_H
#define __REXOS__DRIVERS__VGA_H
#include <common/types.h>
#include <hardwarecomm/interrupts.h>
#include <drivers/driver.h>
#include <hardwarecomm/port.h>
namespace rexos {
	namespace drivers {
        // Starting by sending change mode signal to the Graphics card
        // (V.1.) There are 11 ports
            class VideoGraphicsArray {
                protected:
                    hardwarecomm::Port8Bit miscPort;
                    hardwarecomm::Port8Bit CRTControllerIndexPort;
                    hardwarecomm::Port8Bit CRTControllerDataPort;
                    hardwarecomm::Port8Bit sequenceIndexPort;
                    hardwarecomm::Port8Bit sequenceDataPort;
                    hardwarecomm::Port8Bit graphicsControllerIndexPort;
                    hardwarecomm::Port8Bit graphicsControllerDataPort;
                    hardwarecomm::Port8Bit attributeControllerIndexPort;
                    // (V.2.) Instead of data, here we have read, write, and
                    // reset port
                    hardwarecomm::Port8Bit attributeControllerReadPort;
                    hardwarecomm::Port8Bit attributeControllerWritePort;
                    hardwarecomm::Port8Bit attributeControllerResetPort;
                    // (V.5.) Sends initialization code to corresponding ports
                    void WriteRegisters(common::uint8_t* registers);
                    // (V.6.) Gives us the correct offset for the segment in
                    // the video memory
                    common::uint8_t* GetFrameBufferSegment();
                    
                    // (V.9) PutPixel from V.7. calls this to get the color idx
                    // to get the same index for this color. So, we can give it
                    // more than 8bit colors and the driver hides away the fact
                    // that it cannot represent as many colors. So, multiple
                    // shades defined by user may be represent by the same
                    // shade of a color. The objects on the screen do not know
                    // you only have 8bit color depth on the backend.
                    virtual common::uint8_t GetColorIndex(common::uint8_t r, common::uint8_t g, 
                                        common::uint8_t b);
                public:
                    VideoGraphicsArray();
                    ~VideoGraphicsArray();
                    // (V.3.) A function to set the mode
                    virtual bool SetMode(common::uint32_t width, 
                                        common::uint32_t height, 
                                        common::uint32_t colordepth);
                    // (V.4.) To tell if a mode is supported
                    virtual bool SupportMode(common::uint32_t width, 
                                        common::uint32_t height, 
                                        common::uint32_t colordepth);
                    // (V.7.) Accepts 24bit color code (R,G,B) and puts that at
                    // given coordinate. But VGA will be using an 8bit color
                    // version for now and we cannot set color like this here
                    virtual void PutPixel(common::uint32_t x, common::uint32_t y,
                                        common::uint8_t r, common::uint8_t g, 
                                        common::uint8_t b);
                    // (V.8) PutPixel from V.7. is suppossed to get an index.
                    // The 8Bit color mode uses a table of 256 entries of
                    // different colors. Here you select an index in that table
                    virtual void PutPixel(common::uint32_t x, common::uint32_t y,
                                        common::uint8_t colorIndex);

            };
    };
}
#endif