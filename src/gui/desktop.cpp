#include <gui/desktop.h>
using namespace rexos;
using namespace rexos::gui;
using namespace rexos::common;

Desktop::Desktop(common::int32_t w, common::int32_t h,
    common::uint8_t r, common::uint8_t g, common::uint8_t b):
// Assuming this is a top level desktop, it won't have a parent
// For multi document interface (MDI: form-parent), making desktop the child
// of the window would be a good idea. Would not have to use 0 as x and y
// coordinates if Desktop had parent elements
CompositeWidget(0, 0, 0, w, h, r, g, b),
MouseEventHandler() {
    MouseX = w/2;
    MouseY = h/2;
}
Desktop::~Desktop() {

}

void Desktop::Draw(common::GraphicsContext* gc) {
    CompositeWidget::Draw(gc);
    // White cross mouse cursor
    
    for(int i = 0; i < 4; i++) {
        gc->PutPixel(MouseX-i, MouseY, 0xFF, 0xFF, 0xFF);
        gc->PutPixel(MouseX+i, MouseY, 0xFF, 0xFF, 0xFF);
        gc->PutPixel(MouseX, MouseY-i, 0xFF, 0xFF, 0xFF);
        gc->PutPixel(MouseX, MouseY+i, 0xFF, 0xFF, 0xFF);
    }
}
void Desktop::OnMouseClick(rexos::common::uint8_t button) {
    CompositeWidget::OnMouseClick(MouseX, MouseY, button);
}
void Desktop::OnMouseUnclick(rexos::common::uint8_t button) {
    CompositeWidget::OnMouseUnclick(MouseX, MouseY, button);
}
void Desktop::OnMouseMove(int x, int y) {
    // this is relatively fast
    x /= 4;
    y /= 4;

    int32_t newMouseX = MouseX + x;
    if(newMouseX < 0) newMouseX = 0;
    if(newMouseX >= w) newMouseX = w -1;

    int32_t newMouseY = MouseY + y;
    if(newMouseY < 0) newMouseY = 0;
    if(newMouseY >= h) newMouseY = h -1;

    CompositeWidget::OnMouseMove(MouseX, MouseY, newMouseX, newMouseY);

    MouseX = newMouseX;
    MouseY = newMouseY;
}