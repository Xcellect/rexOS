#include <drivers/keyboard.h>
using namespace rexos::drivers;
using namespace rexos::common;
using namespace rexos::hardwarecomm;

KeyboardEventHandler::KeyboardEventHandler() {

}
void KeyboardEventHandler::OnKeyUp(char c) {

}
void KeyboardEventHandler::OnKeyDown(char c) {

}


// (K.2.) Constructing the KB Driver
KeyboardDriver::KeyboardDriver(InterruptManager* manager, KeyboardEventHandler *handler)
// following is a (data members & base classes) constructor initialization list
// this is done by listing the members between args list and function body
// usually these members dont have default constructors so they have to be initialized
// simplest and efficient way to guarantee all initialization of data members are done before entering the body
: 
InterruptHandler(0x21, manager), // constructing IH with 0x21 (KB int)
dataport(0x60),
commandport(0x64) {
    this->handler = handler; // start the handler
}
KeyboardDriver::~KeyboardDriver() {

}

void printf(char*);
void printfHex(uint8_t);

void KeyboardDriver::Activate() {
    // When the user holds down the key, it'll wait until they release it
    // and it will remove all the keystrokes that might have been there 
    // before
    while(commandport.Read() & 0x1)
        dataport.Read();
    commandport.Write(0xAE);    // a. Activate (KB) interrupts
    commandport.Write(0x20);    // b. Get current state
    // c. Set rightmost bit to 1, since this will be the new state
    // and clear the 5th bit
    uint8_t status = (dataport.Read() | 1) & ~0x10;
    commandport.Write(0x60); // d. Change curr state
    // e. Step b. gives the curr state. We change the curr state and write
    // the changed value back
    dataport.Write(status);

    dataport.Write(0xF4); // Activates keyboard
}
// (K.2.) HandleInterrupt for KB
uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp) {
    // If we have a keystroke we have to fetch it
    uint8_t key = dataport.Read();
    if(handler == 0) {
        return esp;
    }
    static bool shift = false;
    // Ignoring key release interrupts
    
        switch(key)
        {
            
            // Ignoring non-user interrupts
            case 0xFA: break;
            case 0x45: case 0xC5: break;
            // Keymapping
            case 0x02:
                if(shift) handler->OnKeyDown('!'); else handler->OnKeyDown ('1'); break;
            case 0x03:
                if(shift) handler->OnKeyDown('@'); else handler->OnKeyDown ('2'); break;
            case 0x04:
                if(shift) handler->OnKeyDown('#'); else handler->OnKeyDown ('3'); break;
            case 0x05:
                if(shift) handler->OnKeyDown('$'); else handler->OnKeyDown ('4'); break;
            case 0x06:
                if(shift) handler->OnKeyDown('%'); else handler->OnKeyDown ('5'); break;
            case 0x07:
                if(shift) handler->OnKeyDown('^'); else handler->OnKeyDown ('6'); break;
            case 0x08:
                if(shift) handler->OnKeyDown('&'); else handler->OnKeyDown ('7'); break;
            case 0x09:
                if(shift) handler->OnKeyDown('*'); else handler->OnKeyDown ('8'); break;
            case 0x0A:
                if(shift) handler->OnKeyDown('('); else handler->OnKeyDown ('9'); break;
            case 0x0B:
                if(shift) handler->OnKeyDown(')'); else handler->OnKeyDown ('0'); break;
            case 0x0C:
                if(shift) handler->OnKeyDown('_'); else handler->OnKeyDown ('-'); break;
            case 0x0D:
                if(shift) handler->OnKeyDown('+'); else handler->OnKeyDown ('='); break;
            

            case 0x10:
                if(shift) handler->OnKeyDown('Q'); else handler->OnKeyDown ('q'); break;
            case 0x11:
                if(shift) handler->OnKeyDown('W'); else handler->OnKeyDown ('w'); break;
            case 0x12:
                if(shift) handler->OnKeyDown('E'); else handler->OnKeyDown ('e'); break;
            case 0x13:
                if(shift) handler->OnKeyDown('R'); else handler->OnKeyDown ('r'); break;
            case 0x14:
                if(shift) handler->OnKeyDown('T'); else handler->OnKeyDown ('t'); break;
            case 0x15:
                if(shift) handler->OnKeyDown('Y'); else handler->OnKeyDown ('y'); break;
            case 0x16:
                if(shift) handler->OnKeyDown('U'); else handler->OnKeyDown ('u'); break;
            case 0x17:
                if(shift) handler->OnKeyDown('I'); else handler->OnKeyDown ('i'); break;
            case 0x18:
                if(shift) handler->OnKeyDown('O'); else handler->OnKeyDown ('o'); break;
            case 0x19:
                if(shift) handler->OnKeyDown('P'); else handler->OnKeyDown ('p'); break;
            case 0x1A:
                if(shift) handler->OnKeyDown('{'); else handler->OnKeyDown ('['); break;
            case 0x1B:  
                if(shift) handler->OnKeyDown('}'); else handler->OnKeyDown (']'); break;


            case 0x1E:
                if(shift) handler->OnKeyDown('A'); else handler->OnKeyDown ('a'); break;
            case 0x1F:
                if(shift) handler->OnKeyDown('S'); else handler->OnKeyDown ('s'); break;
            case 0x20:
                if(shift) handler->OnKeyDown('D'); else handler->OnKeyDown ('d'); break;
            case 0x21:
                if(shift) handler->OnKeyDown('F'); else handler->OnKeyDown ('f'); break;
            case 0x22:
                if(shift) handler->OnKeyDown('G'); else handler->OnKeyDown ('g'); break;
            case 0x23:
                if(shift) handler->OnKeyDown('H'); else handler->OnKeyDown ('h'); break;
            case 0x24:
                if(shift) handler->OnKeyDown('J'); else handler->OnKeyDown ('j'); break;
            case 0x25:
                if(shift) handler->OnKeyDown('K'); else handler->OnKeyDown ('k'); break;
            case 0x26:
                if(shift) handler->OnKeyDown('L'); else handler->OnKeyDown ('l'); break;
            case 0x27:
                if(shift) handler->OnKeyDown(':'); else handler->OnKeyDown (';'); break;
            case 0x28:
                if(shift) handler->OnKeyDown('\"'); else handler->OnKeyDown ('\''); break;
            
            case 0x2C:
                if(shift) handler->OnKeyDown('Z'); else handler->OnKeyDown ('z'); break;
            case 0x2D:
                if(shift) handler->OnKeyDown('X'); else handler->OnKeyDown ('x'); break;
            case 0x2E:
                if(shift) handler->OnKeyDown('C'); else handler->OnKeyDown ('c'); break;
            case 0x2F:
                if(shift) handler->OnKeyDown('V'); else handler->OnKeyDown ('v'); break;
            case 0x30:
                if(shift) handler->OnKeyDown('B'); else handler->OnKeyDown ('b'); break;
            case 0x31:
                if(shift) handler->OnKeyDown('N'); else handler->OnKeyDown ('n'); break;
            case 0x32:
                if(shift) handler->OnKeyDown('M'); else handler->OnKeyDown ('m'); break;
            case 0x33:
                if(shift) handler->OnKeyDown('<'); else handler->OnKeyDown (','); break;
            case 0x34:
                if(shift) handler->OnKeyDown('>'); else handler->OnKeyDown ('.'); break;
            case 0x35:
                if(shift) handler->OnKeyDown('?'); else handler->OnKeyDown ('/'); break;
            

            case 0x39:
                handler->OnKeyDown(' ');
                break;
            case 0x1C:
                handler->OnKeyDown('\n');
                break;
            case 0x2A: case 0x36:
                shift = true;
                break;
            case 0xAA: case 0xB6:
                shift = false;
                break;
            
            
            default:
            if(key < 0x80) {
                printf("KEYBOARD 0x");
                printfHex(key);
                break;
            }
        }
    
    return esp;
}