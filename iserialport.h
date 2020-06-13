/*
 * Copyright (c) 2020 Dmytro Shestakov
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
#ifndef ISERIALPORT_H
#define ISERIALPORT_H

#include <stdint.h>
#include <string_view>

struct ISerialPort {
    virtual bool AccessCOM() = 0;
    virtual bool OpenCOM() = 0;
    virtual bool CloseCOM() = 0;
    virtual bool SendData(const uint8_t* data, uint32_t size) = 0;
    bool SendByte(uint8_t data)
    {
        return SendData(&data, 1);
    }
    virtual bool ReceiveByte(uint8_t& b) = 0;
    virtual bool ResetStatus() = 0;
    virtual bool Flush() = 0;
    virtual bool setTimeout(uint32_t to) = 0;

    virtual ~ISerialPort() = 0;
};

#endif // ISERIALPORT_H
