#include <gdt.h>
using namespace rexos::common;
using namespace rexos;

// SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type)
GlobalDescriptorTable::GlobalDescriptorTable()
: nullSegmentSelector(0,0,0), 			// constructing the SegmentDescriptor objects
  unusedSegmentSelector(0,0,0),				 
  codeSegmentSelector(0,64*1024*1024,0x9A),
  dataSegmentSelector(0,64*1024*1024,0x92) 
  //	     base ----^	^--- size    ^--flags/type
{
	// Tell the processor to use this table. Tricky bc the CPU
	// expects 6 bytes in a row. Down below we have 8 bytes where
	// the first (LSB 0) 4 bytes are for the address of the table 
	// itself.
	uint32_t i[2];		// 4 byte (32bit) int array
	i[1] = (uint32_t)this; // addr of the table
	i[0] = sizeof(GlobalDescriptorTable) << 16;  // 1st 4 bytes
	// shifting size/limit to last 2 bytes (LSB 0) 
	// shift it to the left bc it's high bytes
	
	// in-line assembler to tell the processor to use this table
	// ldgdt = load gdt from the passed address of this table
	asm volatile("lgdt (%0)": :"p" ((uint8_t *)i+2)  );
	// GDT is at i[1]. So address of i[1] is i+2 which is i + 2 bytes (16bits). 
	// Because, here the ptr is 8 bit (1 byte). size(i) = type*elem# = 8 bytes.
	// CPU expects 6 bytes so passing the address starting at 48th (LSB 0) bit.
	// Note: simply passing the GDT's address to the processor is not enough.
	// Assumption: CPU expects the address to be entirely 6 bytes long and
	// also expects the limit to be after the address.
}

GlobalDescriptorTable::~GlobalDescriptorTable() {
}
// Returns the OFFSETS of code and data segments. We can do that by
// subtracting the address of this table from the addr of the respective
// segment descriptor objects
uint16_t GlobalDescriptorTable::DataSegmentOffset() {
	return (uint8_t*)&dataSegmentSelector - (uint8_t*)this;
}
uint16_t GlobalDescriptorTable::CodeSegmentOffset() {
	return (uint8_t*)&codeSegmentSelector - (uint8_t*)this;
}


// Set the entries according to the parameters

GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(uint32_t base, 
		uint32_t limit, uint8_t flags) {
// Time to distinguish a few cases
	uint8_t* target = (uint8_t*)this;
// if limit is small (16bit)
	if(limit<= 65536) {
		target[6] = 0x40;	// tells the CPU this is a 16bit entry
	} else {
		/* Problem: the limit bytes (2.5 bytes) is not sufficient to 
		   address the whole RAM (i.e. 4GB)
		   Solution: the number in the limit in the byte 0, 1 and 6 is 
		   multiplied by 2^12 (equivalent to left shifting 12 places). 
		  
		  	 ___ _ ___ ___
		    	|   | |   |   |
		   	 vir 6  1   0
			     ^^^^^^^^^ right shifting these 20 bits 12 places
				12+20=32
		   The base parameter is uint32 so it's allocated 4 bytes. The 
		   most significant 20 bits are used and the rest 12 bits are cut off. 
		   
		   Problem: only legal if these 12 bits are all 1's. setting them 
		   all to 1 would put the base table entry beyond the desired 
		   location (in code segment) and/or overlap with other segments.
		   Solution: Subtract 1 from the 20 bit number and legally increment
		   the remaining 12 bits until they are all 1's.
		   Con: Leaves an unused space here.
		*/
		if((limit & 0xFFF) != 0xFFF)	// if last 12 bits are not all 1's
			limit = (limit >> 12)-1;	// shift it by 12 and subtract 1
		else
			limit = limit >> 12;		// otherwise, just shift it by 12
		// now the limit has a legal value
		target[6] = 0xC0;	// tells the CPU this is not 16bit
	}
	/* 	The 8 byte GDT:
		 _______ _______ _______ _______ _______ _______ _______ _______
		|  ptr  | f | l | flags |  ptr  |  ptr  |  ptr  | limit | limit |
		    7	    6       5	    4	    3	    2	    1	    0
			    ^	    ^---------- access rights
			a. High bits: flags
			b. Low bits: limit 
	*/
	// the AND operation preserves info so it effectively selects last 8 bits only
	target[0] = limit & 0xFF;
	// left shifting (discarding) last used 8 bits (12 bits remaining)
	target[1] = (limit >> 8) & 0xFF;
	// discarding last used 16 bits (4 bits remaining)	
	target[6] |= (limit >> 16) & 0xF;	
	// eg. limit (size) is 1046239 bytes (2.5 byte/20 bit uint)
	// 1046239(dec) = 1111 11110110 11011111 (bin)
	//				   ___ _______ _______ 
	//				  | l | limit | limit |
	//				    6	  1	  0
	// target[0] will have 1111 1111 0110 1101 1111 & 1111 1111 = 1101 1111
	// target[1] will have 1111 1111 0110 & 1111 1111 = 1111 0110
	// target[6] will have 1111 & 1111 = 1111
	target[2] =  base & 0xFF;
	target[3] = (base >> 8) & 0xFF;
	target[4] = (base >> 16) & 0xFF;
	target[7] = (base >> 24) & 0xFF;
	/* eg. base is at memory address 2651568493 (4 byte/32 bit uint)
	   2121568493 (dec) = 10011110 00001011 10111001 01101101 (bin)
	  		  ________ ________ ________ ________
			 |   ptr  |  ptr   |   ptr  |   ptr  |
			      7       4	        3        2
	 * target[2] gets 10011110 00001011 10111001 01101101 & 11111111 = 11101101
	 * target[3] gets 10011110 00001011 10111001 & 11111111 = 10010000
	 * target[4] gets 10011110 00001011 & 11111111 = 01110100
	 * target[7] gets 10011110 & 11111111 = 1111110
	 
	Bitshifting n places works in the following manner:
	* Based on the shift register, ie. Serial In-Serial Out, each bit is 
	  shifted right 1 stage each time data advance is brought HIGH. On each 
	  data advance, the left most bit (ie. data in) is shifted in the first 
	  flip-flop's output and the right most bit (ie. data out) is lost. 
		
	  In Parallel In-Serial Out, the data is inputted similar to SISO but the 
	  data can be read of simultaneously or shifted.
	  Here, the flip-flops are edge triggered (square wave - hence called edge 
	  - transition between LOW/HIGH voltage). All flip-flops work at a specific
	  CPU frequency. So, an input bit makes its way down the nth flip-flop's 
	  output after n clock cycles.

	  As for Parallel In-Serial Out, the input bits are inputted from D1 
	  through Dn (MSB 0) wires concurrently. Data is read when the Write/Shift 
	  wire is on LOW. Data is shifted when it's on HIGH and CPU is clocked.

	  Therefore, bitshifting n places means applying edge triggers and/or 
	  n clock cycles.
	eg.
	  uint32_t base has space for 32 bits
	 	
		10011110 00001011 10111001 01101101
		^ right shifting 8 places advances the bits to the 24th flip-flop (LSB 0
		bit numbering) of the register after 8 edge triggers/cycles
	    
		00000000 10011110 00001011 10111001 | 01101101
		^ preceding 8 bits are		      ^ last 8 bits are
		  padded with 0's			discarded
	
	* For a number m,
		m << n == m * 2^n
		m >> n == m / 2^n
	  
	*/
	target[5] = flags;	// access flags

}
/* Time to do things in reverse.
   Take the corresponding bytes if we request the base and limit. We might have
   to change the limit so we have to compute it again.
 */
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
 uint8_t* target = (uint8_t*)this;
 // first take the low 4 bits (right-most) of the shared (6th) byte
 uint32_t result = target[6] & 0xF;
 // then we do similar bitshifting as before
 result = (result << 8) + target[1];
 result = (result << 8) + target[0];
 if((target[6] & 0xC0) == 0xC0)		// only shift if it's not 16bit
 	result = (result << 12) | 0xFFF;
 return result;

}

