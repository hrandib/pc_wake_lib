#pragma once

#include <stdint.h>

namespace Utils {
	constexpr static uint16_t htons(uint16_t val)
	{
		return (val & 0xFF00) >> 8
			| (val & 0x00FF) << 8;
	}
	constexpr static uint32_t htonl(uint32_t val)
	{
		return  (val & 0xFF000000) >> 24
			| (val & 0x00FF0000) >> 8
			| (val & 0x0000FF00) << 8
			| (val & 0x000000FF) << 24;
	}
	constexpr static uint16_t ntohs(uint16_t val)
	{
		return htons(val);
	}
	constexpr static uint32_t ntohl(uint32_t val)
	{
		return  htonl(val);
	}
}//Utils