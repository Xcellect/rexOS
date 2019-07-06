#ifndef __REXOS__HARDWARECOMM__PORT_H
#define __REXOS__HARDWARECOMM__PORT_H
#include <common/types.h>

namespace rexos {
	namespace hardwarecomm {
		class Port {
			protected:
				rexos::common::uint16_t portnumber;
				Port(rexos::common::uint16_t portnumber);
				~Port();
		};

		class Port8Bit : public Port {
			public:
				Port8Bit(rexos::common::uint16_t portnumber);
				~Port8Bit();
				virtual void Write(rexos::common::uint8_t data);
				virtual rexos::common::uint8_t Read();
		};
		// extends the Port8Bit class
		class Port8BitSlow : public Port8Bit {

			public:
				Port8BitSlow(rexos::common::uint16_t portnumber);
				~Port8BitSlow();
				virtual void Write(rexos::common::uint8_t data);
		};
		class Port16Bit : public Port {

			public:
				Port16Bit(rexos::common::uint16_t portnumber);
				~Port16Bit();
				virtual void Write(rexos::common::uint16_t data);
				virtual rexos::common::uint16_t Read();
		};
		class Port32Bit : public Port {

			public:
				Port32Bit(rexos::common::uint16_t portnumber);
				~Port32Bit();
				virtual void Write(rexos::common::uint32_t data);
				virtual rexos::common::uint32_t Read();
		};
	}
}

#endif
