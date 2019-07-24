#include <gui/window.h>
using namespace rexos::common;
using namespace rexos::gui;
Window::Window(Widget* parent, 
            int32_t x, int32_t y, int32_t w, int32_t h,
            int8_t r, int8_t g, int8_t b):
CompositeWidget(parent, x, y, w, h, r,g, b) {
    Dragging = false;
}
Window::~Window() {}
void Window::OnMouseClick(int32_t x, int32_t y, uint8_t button) {
    Dragging = button == 1;
    CompositeWidget::OnMouseClick(x, y, button);
}
void Window::OnMouseUnclick(int32_t x, int32_t y, uint8_t button) {
    Dragging = false;
    CompositeWidget::OnMouseUnclick(x,y,button);
}
void Window::OnMouseMove(int32_t oldx, int32_t oldy, int32_t newx, int32_t newy) {
    if(Dragging) {
        this->x += newx - oldx;
        this->y += newy - oldy;
    }
    CompositeWidget::OnMouseMove(oldx,oldy,newx,newy);
}