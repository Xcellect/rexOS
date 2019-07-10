#ifndef __REXOS__HARDWARECOMM__PORT_H	// user protection (namespace/scope)
#define __REXOS__HARDWARECOMM__PORT_H	// declared variables from port.h 
#include <common/types.h>

namespace rexos {
	namespace hardwarecomm {
		// base class for ports that knows its own port number
		class Port {
			protected:
				// port number is always a 16 bit integer
				rexos::common::uint16_t portnumber;
				// protected ctor so we can't instantiate this
				// since it's a virtual base class
				Port(rexos::common::uint16_t portnumber);
				~Port();
		};
		// Note: all the functions are virtual (can be overridden)
		// 8 bit port
		class Port8Bit : public Port {
			public:
				Port8Bit(rexos::common::uint16_t portnumber);
				~Port8Bit();
				virtual void Write(rexos::common::uint8_t data);
				virtual rexos::common::uint8_t Read();
		};
		// extends the Port8Bit class to inherit the Read() function 
		// so we don't have to override that
		// slower than regular 8bit port so it needs its own Write() 
		// function
		class Port8BitSlow : public Port8Bit {

			public:
				Port8BitSlow(rexos::common::uint16_t portnumber);
				~Port8BitSlow();
				virtual void Write(rexos::common::uint8_t data);
		};
		// 16 bit port
		class Port16Bit : public Port {

			public:
				Port16Bit(rexos::common::uint16_t portnumber);
				~Port16Bit();
				virtual void Write(rexos::common::uint16_t data);
				virtual rexos::common::uint16_t Read();
		};
		// 32 bit port
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
