#include <hardwarecomm/port.h>
using namespace rexos::common;
using namespace rexos::hardwarecomm;
// ctor of the base class will just store the port number
Port::Port(uint16_t portnumber) {
	this->portnumber = portnumber;
}
Port::~Port() {
}

// ctor of the 8 bit port will forward port number to the base
Port8Bit::Port8Bit(uint16_t portnumber)
: Port(portnumber) {
}

Port8Bit::~Port8Bit() {
}
// read and write asm instructions are self-explanatory
void Port8Bit::Write(uint8_t data) {
	__asm__ volatile("outb %0, %1" :: "a" (data), "Nd" (portnumber));
}

uint8_t Port8Bit::Read() {
	uint8_t result;
	__asm__ volatile("inb %1, %0" : "=a" (result) : "Nd" (portnumber));
	return result;
}
/////////////////////////////////////////////////////////

Port8BitSlow::Port8BitSlow(uint16_t portnumber)
: Port8Bit(portnumber) {

}

Port8BitSlow::~Port8BitSlow() {
}
// adding some garbage instructions to make the program wait until 
// the port is done writing the data
// adding 2 garbage instructions to make the operation slower
void Port8BitSlow::Write(uint8_t data) {
	__asm__ volatile("outb %0, %1\njmp 1f\n1:jmp 1f\n1:" :: "a" (data), "Nd" (portnumber));
}

// Read method has been inherited from the Port8Bit class

/////////////////////////////////////////////////////////
Port16Bit::Port16Bit(uint16_t portnumber)
: Port(portnumber) {

}

Port16Bit::~Port16Bit() {
}

void Port16Bit::Write(uint16_t data) {
	__asm__ volatile("outw %0, %1" :: "a" (data), "Nd" (portnumber));
}

uint16_t Port16Bit::Read() {
	uint16_t result;
	__asm__ volatile("inw %1, %0" : "=a" (result) : "Nd" (portnumber));
	return result;
}

/////////////////////////////////////////////////////////
Port32Bit::Port32Bit(uint16_t portnumber)
: Port(portnumber) {

}

Port32Bit::~Port32Bit() {
}

void Port32Bit::Write(uint32_t data) {
	__asm__ volatile("outl %0, %1" :: "a" (data), "Nd" (portnumber));
}

uint32_t Port32Bit::Read() {
	uint32_t result;
	__asm__ volatile("inl %1, %0" : "=a" (result) : "Nd" (portnumber));
	return result;
}
