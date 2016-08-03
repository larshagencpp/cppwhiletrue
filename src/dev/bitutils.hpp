#pragma once
#include <cassert>
#include <cstdint>

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanReverse64)
#endif

namespace cwt {
	template<typename T>
	T bit_scan_reverse_non_negative(T value) = delete;
	
	uint64_t bit_scan_reverse_non_negative(uint64_t value) {
		assert(value > 0);
		unsigned long array_index;
		static_assert(sizeof(void*) == 8, "Only 64-bit build is supported.");
#ifdef _MSC_VER
		_BitScanReverse64(&array_index, value);
#else
		array_index = 63 - static_cast<uint32_t>(__builtin_clzl(value));
#endif
		return array_index;
	}
}