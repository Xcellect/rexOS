#ifndef __REXOS__GDT_H			// protection
#define __REXOS__GDT_H
#include <common/types.h>
namespace rexos {
	/* 
	Purpose: (G.1.) Setting up a global descriptor table to define memory segments for
	the IDT or the interrupt handler. This helps the IDT switch back and forth 
	between the kernel and the userspace memory segments on the event of a H/W
	interrupt.
	
	GDT Layout in memory:
		Entries in GDT are backwards compatible to i386 processors w/ 16 bit 
		addr spaces. Entry in the table is 8 bytes long. For 16 bit processors 
		the following layout was enough:
		
		|  ptr  |  ptr  | limit | limit |
		
		But with only 2 bytes for ptr, you could only address 64 KB so this was
		expanded in the following manner:
		 _______ _______ _______ _______ _______ _______ _______ _______
		|  ptr  | f | l | flags |  ptr  |  ptr  |  ptr  | limit | limit |
		    7       6       5       4       3       2       1       0
			    ^       ^---------- access rights
			1. High bits: flags/type
			2. Low bits: limit
	 */
	class GlobalDescriptorTable {
		public:
			class SegmentDescriptor {
				private:
					rexos::common::uint16_t limit_lo;		// 0, 1
					rexos::common::uint16_t base_lo;		// 2, 3
					rexos::common::uint8_t base_hi;			// 4
					rexos::common::uint8_t type;			// 5
					rexos::common::uint8_t flags_limit_hi;  // 6
					rexos::common::uint8_t base_vhi;		// 7
				public:
					// a ctor that gets regular base and limits as 32 bit 
					// integers and aligns these data properly
					SegmentDescriptor(rexos::common::uint32_t base, 
						rexos::common::uint32_t limit, 
							rexos::common::uint8_t type);
					// methods that returns ptr and the limit computing 
					// spread out data
					rexos::common::uint32_t Base();
					rexos::common::uint32_t Limit();
			} __attribute__((packed));		// compiler's not allowed to shift 
			// bytes for optimization.
			// the above keyword makes this class byte perfect
		private:
		// SegmentDescriptor is a class for entries
		// this is not an array. only putting explicit/specific entries
			SegmentDescriptor nullSegmentSelector;		// empty
			SegmentDescriptor unusedSegmentSelector;	// unused
			SegmentDescriptor codeSegmentSelector;		// code/text segment
			SegmentDescriptor dataSegmentSelector;		// data segment

		public:
			GlobalDescriptorTable();
			~GlobalDescriptorTable();
			// the following will return the offset for the code/dataSegmentSelector
			rexos::common::uint16_t CodeSegmentOffset();
			rexos::common::uint16_t DataSegmentOffset();
	};
}


#endif
