#ifndef __REXOS__GUI__DESKTOP_H
#define __REXOS__GUI__DESKTOP_H

#include <drivers/mouse.h>
#include <gui/widget.h>

namespace rexos {
    namespace gui {
        class Desktop : public CompositeWidget, public drivers::MouseEventHandler {
            // The desktop will work like the composite widget, we need to
            // override the functions from MouseEventHandler here. Bc we only
            // get relative movements from mouse so the desktop will turn
            // relative to absolute cooordinates of mouse
            protected:
                common::uint32_t MouseX;
                common::uint32_t MouseY;
            public:
                Desktop(common::int32_t w, common::int32_t h,
                    common::uint8_t r, common::uint8_t g, common::uint8_t b);
                ~Desktop();

                void Draw(common::GraphicsContext* gc);
                virtual void OnMouseClick(rexos::common::uint8_t button);
                virtual void OnMouseUnclick(rexos::common::uint8_t button);
                virtual void OnMouseMove(int x, int y);
        };
    }
}

#endif