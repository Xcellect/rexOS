#include <drivers/driver.h>
using namespace rexos::drivers;

        Driver::Driver() {

        }
        Driver::~Driver() {

        }

        void Driver::Activate() {

        }
        // when you start the os you enumerate the hardware
        // you dont know the state of the devices so you reset them
        // wait about 15ms after that
        // when the devs start to run you know the state they are in
        // it can be theoretically possible that, 
        // you start and shutdown the os and it goes back in bootloader
        // the bootloader somehow decides to run your os 
        // (linux kernel can do that so you can do kernel updates w/o rebooting system)
        // * what if the os ran before you cleanup everything?
        // * the bootloader can also be in an uncertain state
        // maybe the hw isnt in the state you expect
        int Driver::Reset() {
            return 0;
        }

        void Driver::Deactivate() {

        }



    // since we dont have dynamic memory management yet
    // we'll make a fixed length driver
DriverManager::DriverManager() {
    numDrivers = 0;
}
void DriverManager::AddDriver(Driver* drv) {
    drivers[numDrivers] = drv;
    numDrivers++;
    
}

void DriverManager::ActivateAll() {
    for(int i = 0; i< numDrivers; i++) {
        drivers[i]->Activate();
    }
}