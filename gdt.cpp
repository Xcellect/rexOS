#include "gdt.h"

GlobalDescriptorTable::GlobalDescriptorTable()
: nullSegmentSelector(0,0,0), 
  unusedSegmentSelector(0,0,0),
  codeSegmentSelector(0,64*1024*1024,0x9A), 
  dataSegmentSelector(0,64*1024*1024,0x92)
{
	// Tell the processor to use this table. Tricky bc the CPU
	// expects 6 bytes in a row. Down below we have 8 bytes where
	// the first byte is the address of the table itself.
	uint32_t i[2];
	i[1] = (uint32_t)this; // addr of the table
	i[0] = sizeof(GlobalDescriptorTable) << 16;  // last 4 bytes are the high bytes of the segment integer. shift it to the left bc it's high bytes
	

	// in-line assembler to tell the processor to use this table
	asm volatile("lgdt (%0)": :"p" (((uint8_t *) i)+2) );

}

GlobalDescriptorTable::~GlobalDescriptorTable() {
}
// Returns the offsets of code and data segments. We can do that by
// subtracting the address of this table from the addr of the respective
// segment selectors
uint16_t GlobalDescriptorTable::DataSegmentSelector() {
	return (uint8_t*)&dataSegmentSelector - (uint8_t*)this;
}
uint16_t GlobalDescriptorTable::CodeSegmentSelector() {
	return (uint8_t*)&codeSegmentSelector - (uint8_t*)this;
}


// Set the entries according to the parameters

GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t flags) {
// Time to distinguish a few cases
	uint8_t* target = (uint8_t*)this;

	if(limit<= 65536) {
		target[6] = 0x40;
	} else {
		if((limit & 0xFFF) != 0xFFF)
			limit = (limit >> 12)-1;
		else
			limit = limit >> 12;

		target[6] = 0xC0;
	}
	target[0] = limit & 0xFF;
	target[1] = (limit >> 8) & 0xFF;
	target[6] |= (limit >> 16) & 0xF;

	target[2] =  base & 0xFF;
	target[3] = (base >> 8) & 0xFF;
	target[4] = (base >> 16) & 0xFF;
	target[7] = (base >> 24) & 0xFF;

	target[5] = flags;

}

// Base: All the 4 bytes are preserved
uint32_t GlobalDescriptorTable::SegmentDescriptor::Base() {
 uint8_t* target = (uint8_t*)this;
 uint32_t result = target[7];  // Take the 7th byte and successively left shift
 result = (result << 8) + target[4];
 result = (result << 8) + target[3];
 result = (result << 8) + target[2];
 return result;
}


uint32_t GlobalDescriptorTable::SegmentDescriptor::Limit() {
// first take the low 4 bits of the shared byte
 uint8_t* target = (uint8_t*)this;
 uint32_t result = target[6] & 0xF;  
 result = (result << 8) + target[1];
 result = (result << 8) + target[0];
 if((target[6] & 0xC0) == 0xC0)
 	result = (result << 12) | 0xFFF;
 return result;

}

