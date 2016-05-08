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

	class ProgressBar
	{
	private:
		constexpr static size_t DEFAULT_BAR_LENGTH = 50;
		const size_t maxVal_;
		const size_t barLength_;
		std::string valDimension_;
		const size_t halfScaleDiv = maxVal_ / (barLength_ * 2);
	public:
		ProgressBar(size_t maxVal, const char* valDimension, size_t barLength = 0) : maxVal_(maxVal ? maxVal : 1),
									barLength_(barLength ? barLength : DEFAULT_BAR_LENGTH), valDimension_(valDimension)
		{  }
		void Update(size_t currentValue)
		{
			using std::cout;
			using std::endl;
			auto position = ((currentValue + halfScaleDiv) * barLength_) / maxVal_ ;
			cout << "\r[";
			for(size_t i{}; i < position; ++i) {
				cout << '=';
			}
			for(size_t i = position; i < barLength_; ++i)	{
				cout << ' ';
			}
			cout << "] " << position * 100 / barLength_ << "% " << currentValue << ' ' << valDimension_;
			if(currentValue == maxVal_) {
				cout << endl;
			}
		}
	};

}//Utils