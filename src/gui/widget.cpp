#include <gui/widget.h>
using namespace rexos::gui;
using namespace rexos::common;

Widget::Widget(Widget* parent, int32_t x, int32_t y, int32_t w,
                int32_t h, int8_t r, int8_t g, 
                int8_t b):
KeyboardEventHandler() {
                    this->parent = parent;
                    this->x = x;
                    this->y = y;
                    this->w = w;
                    this->h = h;
                    this->r = r;
                    this->g = g;
                    this->b = b;
                    this->Focusable = true;
}
Widget::~Widget() {

}
void Widget::GetFocus(Widget* widget) {
    if(parent != 0) {
        parent->GetFocus(widget);
    }
}
void Widget::ModelToScreen(int32_t &x, int32_t &y) {
    // (W.1.) If parent is not 0 we pass the focus to the parent
    if(parent!=0) {
        parent->ModelToScreen(x, y);
    }
    x += this->x;
    y += this->y;
}
void Widget::Draw(GraphicsContext* gc) {
    int X = 0;  // set this to 0 bc ModelToScreen adds its own x and y
    int Y = 0;
    ModelToScreen(X, Y);
    gc->FillRectangle(X,Y, w,h, r,g,b); // default implementation
}



void Widget::OnMouseClick(int32_t x, int32_t y, uint8_t button) {
    if(Focusable) {
        GetFocus(this);
    }
}
bool Widget::ContainsCoordinate(int32_t x, int32_t y) {
    // (C.W.3) Checking if the passed values are smaller than or equal to this x and
    // smaller than this.x + this.width
    // Works even though this x and y are relative coords because
    // We're passing the relative coords at (C.W.1) thru (C.W.2)
    return this->x <= x && x < this->x + this->w &&
           this->y <= y && y < this->y + this->h;
}
void Widget::OnMouseUnclick(int32_t x, int32_t y, uint8_t button) {

}
void Widget::OnMouseMove(int32_t oldx, int32_t oldy, int32_t newx, int32_t newy) {

}


CompositeWidget::CompositeWidget(Widget* parent, int32_t x, int32_t y, int32_t w,
                    int32_t h, int8_t r, int8_t g, int8_t b) 
: Widget(parent, x, y, w, h, r, g, b)                    {
                        focusedChild = 0;
                        numChildren = 0;
}
CompositeWidget::~CompositeWidget() {
}



void CompositeWidget::GetFocus(Widget* widget) {
    this->focusedChild = widget;
    if(parent != 0 ) {
        parent->GetFocus(this);
    }
}
bool CompositeWidget::AddChild(Widget* child) {
    // Not thread safe but it's synchronus still
    if(numChildren >= 100)
        return false;
    children[numChildren++] = child;
    return true;
}
void CompositeWidget::Draw(GraphicsContext* gc) {
    Widget::Draw(gc);
    for(int i = numChildren-1; i>=0; --i)
        children[i]->Draw(gc);  //Draws the children
}
void CompositeWidget::OnMouseClick(int32_t x, int32_t y, uint8_t button) {
    // (C.W.1) Iterate through the children and pass the event to the child that
    // contains the coodinates
    // Children w smaller indices are closer to the front
    // They are encountered in order and therefore, only the child whose
    // coordinate the most immediate to the front gets selected 
    for(int i = 0; i<numChildren; ++i)
        if(children[i]->ContainsCoordinate(x - this->x, y - this->y)) {
            // Passing relative coords
            children[i]->OnMouseClick(x - this->x, y - this->y, button);  //Draws the children
            break;
        }
}
void CompositeWidget::OnMouseUnclick(int32_t x, int32_t y, uint8_t button) {
    for(int i = 0; i<numChildren; ++i)
        if(children[i]->ContainsCoordinate(x - this->x, y - this->y)) {
            // Passing relative coords
            children[i]->OnMouseUnclick(x - this->x, y - this->y, button);  //Draws the children
            break;
        }
}

// (C.W.2) If mouse leaves an element, it only gets the first part ----^
// If mouse enters a child, it gets the second part ----v
void CompositeWidget::OnMouseMove(int32_t oldx, int32_t oldy, int32_t newx, int32_t newy) {
    int firstchild = -1;
    for(int i = 0; i<numChildren; ++i)
        if(children[i]->ContainsCoordinate(oldx - this->x, oldy - this->y)) {
            // subtracting this.x and this.y turns them into relative coords
            children[i]->OnMouseMove(oldx - this->x, oldy - this->y, newx - this->x, newy - this->y);  //Draws the children
            firstchild = i;
            break;
        }
            
    for(int i = 0; i<numChildren; ++i)
        if(children[i]->ContainsCoordinate(newx - this->x, newy - this->y)) {
            if(firstchild != i)
                children[i]->OnMouseMove(oldx - this->x, oldy - this->y, newx - this->x, newy - this->y);  //Draws the children
            break;
        }
}   // This could by more elegant by: To check the elements at entering and
    // leaving are the same. Call OnMouseMove if so, else call OnMouseLeave
    // on the first and OnMouseEnter on the seconds widget

// Overriding the OnKeyUp/Down function from keyboard in the Composite widget
// So Composite widget like desktop can pass the keystroke to the focused child
void CompositeWidget::OnKeyDown(char str) {
    if(focusedChild != 0)
        focusedChild->OnKeyDown(str);
}
void CompositeWidget::OnKeyUp(char str) {
    if(focusedChild != 0)
        focusedChild->OnKeyUp(str);
}