#ifndef __REXOS__GDT_H
#define __REXOS__GDT_H
#include <common/types.h>
namespace rexos {
	class GlobalDescriptorTable {
		public:
			class SegmentDescriptor {
				private:
					rexos::common::uint16_t limit_lo;
					rexos::common::uint16_t base_lo;
					rexos::common::uint8_t base_hi;
					rexos::common::uint8_t type;
					rexos::common::uint8_t flags_limit_hi;
					rexos::common::uint8_t base_vhi;
				public:
					SegmentDescriptor(rexos::common::uint32_t base, rexos::common::uint32_t limit, rexos::common::uint8_t type);
					rexos::common::uint32_t Base();
					rexos::common::uint32_t Limit();
			} __attribute__((packed));
		private:
			SegmentDescriptor nullSegmentSelector;
			SegmentDescriptor unusedSegmentSelector;
			SegmentDescriptor codeSegmentSelector;
			SegmentDescriptor dataSegmentSelector;

		public:
			GlobalDescriptorTable();
			~GlobalDescriptorTable();

			rexos::common::uint16_t CodeSegmentSelector();
			rexos::common::uint16_t DataSegmentSelector();
	};
}


#endif
