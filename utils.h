/*
 * Copyright (c) 2016 Dmytro Shestakov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <stdint.h>

namespace Utils {
constexpr static uint16_t htons(uint16_t val)
{
    return (val & 0xFF00) >> 8 | (val & 0x00FF) << 8;
}
constexpr static uint32_t htonl(uint32_t val)
{
    return (val & 0xFF000000) >> 24 | (val & 0x00FF0000) >> 8 | (val & 0x0000FF00) << 8 | (val & 0x000000FF) << 24;
}
constexpr static uint16_t ntohs(uint16_t val)
{
    return htons(val);
}
constexpr static uint32_t ntohl(uint32_t val)
{
    return htonl(val);
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
    ProgressBar(size_t maxVal, const char* valDimension, size_t barLength = 0) :
      maxVal_(maxVal ? maxVal : 1), barLength_(barLength ? barLength : DEFAULT_BAR_LENGTH), valDimension_(valDimension)
    { }
    void Update(size_t currentValue)
    {
        using std::cout;
        using std::endl;
        auto position = ((currentValue + halfScaleDiv) * barLength_) / maxVal_;
        cout << "\r[";
        for(size_t i{}; i < position; ++i) {
            cout << '=';
        }
        for(size_t i = position; i < barLength_; ++i) {
            cout << ' ';
        }
        cout << "] " << position * 100 / barLength_ << "% " << currentValue << ' ' << valDimension_;
        if(currentValue == maxVal_) {
            cout << endl;
        }
    }
};

template<bool, typename T1, typename T2>
struct select_if
{
    using type = T1;
};
template<typename T1, typename T2>
struct select_if<false, T1, T2>
{
    using type = T2;
};
template<bool value, typename T1, typename T2>
using select_if_t = typename select_if<value, T1, T2>::type;

} // Utils