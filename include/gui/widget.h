#ifndef __REXOS__GUI__WIDGET_H
#define __REXOS__GUI__WIDGET_H
#include <common/types.h>
#include <common/graphicscontext.h>
#include <drivers/keyboard.h>

namespace rexos {
    namespace gui { 
        // If we derive the Widget class from KeyboardHandler, we can inherit 
        // OnKeyUp and OnKeyDown functions. Then attach Widget/Desktop 
        // directly to the keyboard
        class Widget : public drivers::KeyboardEventHandler {
            protected:
                Widget* parent;
                rexos::common::int32_t x;
                rexos::common::int32_t y;
                rexos::common::int32_t w;
                rexos::common::int32_t h;
                
                rexos::common::int8_t r;
                rexos::common::int8_t g;
                rexos::common::int8_t b;
                bool Focusable;
            public:
                Widget(Widget* parent, 
                    rexos::common::int32_t x, rexos::common::int32_t y, rexos::common::int32_t w, rexos::common::int32_t h,
                    rexos::common::int8_t r, rexos::common::int8_t g, rexos::common::int8_t b);
                ~Widget();

                virtual void GetFocus(Widget* widget);
                virtual void ModelToScreen(rexos::common::int32_t &x, rexos::common::int32_t &y);

                virtual bool ContainsCoordinate(rexos::common::int32_t x, rexos::common::int32_t y);
                virtual void Draw(common::GraphicsContext* gc);
                virtual void OnMouseClick(rexos::common::int32_t x, rexos::common::int32_t y, rexos::common::uint8_t button);
                virtual void OnMouseUnclick(rexos::common::int32_t x, rexos::common::int32_t y, rexos::common::uint8_t button);
                virtual void OnMouseMove(rexos::common::int32_t oldx, rexos::common::int32_t oldy, rexos::common::int32_t newx, rexos::common::int32_t newy);

                
                
        };
        // CompositeWidget is derived from MouseEventHanglers
        // (W.3.) CompositeWidget will overwride most methods
        class CompositeWidget : public Widget {
            private:
                Widget* children[100];
                int numChildren;
                Widget* focusedChild;
            public:
                CompositeWidget(Widget* parent, 
                    rexos::common::int32_t x, rexos::common::int32_t y, rexos::common::int32_t w,
                    rexos::common::int32_t h, rexos::common::int8_t r, rexos::common::int8_t g, rexos::common::int8_t b);
                ~CompositeWidget();

                virtual void GetFocus(Widget* widget);
                // A method to add child bc children are private
                virtual bool AddChild(Widget* child);
                // Don't have to override ModelToScreen
                virtual void Draw(common::GraphicsContext* gc);
                virtual void OnMouseClick(rexos::common::int32_t x, rexos::common::int32_t y, rexos::common::uint8_t button);
                virtual void OnMouseUnclick(rexos::common::int32_t x, rexos::common::int32_t y, rexos::common::uint8_t button);
                virtual void OnMouseMove(rexos::common::int32_t oldx, rexos::common::int32_t oldy, rexos::common::int32_t newx, rexos::common::int32_t newy);

                virtual void OnKeyDown(char);
                virtual void OnKeyUp(char);

        };
    }
}

#endif