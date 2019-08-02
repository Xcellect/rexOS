#ifndef __REXOS__COMMON__TYPES_H
#define __REXOS__COMMON__TYPES_H
namespace rexos {
	namespace common {
		typedef char int8_t;
		typedef unsigned char uint8_t;

		typedef short int16_t;
		typedef unsigned short uint16_t;

		typedef int int32_t;
		typedef unsigned int uint32_t;

		typedef long long int int64_t;
		typedef unsigned long long int uint64_t;

		typedef const char* string;
		/* 32 bit systems can address upto 2^32 = 4,294,967,294 individual byte
		and the max size can be 4,294,967,294 bytes = 4 GiB. Hence, size type
		is a 32 bit int. */
		typedef uint32_t size_t;
	}
}
#endif
