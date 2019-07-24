#ifndef __REXOS__GUI__WINDOW_H
#define __REXOS__GUI__WINDOW_H

#include <drivers/mouse.h>
#include <gui/widget.h>

namespace rexos {
    namespace gui {
        class Window : public CompositeWidget {
            protected:
                bool Dragging;
            public:
                Window(Widget* parent, 
                        rexos::common::int32_t x, rexos::common::int32_t y, rexos::common::int32_t w, rexos::common::int32_t h,
                        rexos::common::int8_t r, rexos::common::int8_t g, rexos::common::int8_t b);
                ~Window();
                void OnMouseClick(rexos::common::int32_t x, rexos::common::int32_t y, rexos::common::uint8_t button);
                void OnMouseUnclick(rexos::common::int32_t x, rexos::common::int32_t y, rexos::common::uint8_t button);
                void OnMouseMove(rexos::common::int32_t oldx, rexos::common::int32_t oldy, rexos::common::int32_t newx, rexos::common::int32_t newy);
        };
    }
}

#endif