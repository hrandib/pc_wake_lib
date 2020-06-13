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
#ifndef SERIALPORT_WIN_H
#define SERIALPORT_WIN_H

#include "iserialport.h"
#include <string>

#include <windows.h>

class SerialPort : public ISerialPort {
    // ISerialPort interface
public:
    using stringv = std::string_view;
    SerialPort(stringv portPath, uint32_t baud);
    bool AccessCOM() override;
    bool OpenCOM() override;
    bool CloseCOM() override;
    bool SendData(const uint8_t* data, uint32_t size) override;
    bool ReceiveByte(uint8_t& b) override;
    bool ResetStatus() override;
    bool Flush() override;
    bool setTimeout(uint32_t to) override;

private:
    const std::string portName_;
    uint32_t baud_;
    HANDLE        hCom;       //COM handle
    DCB           dcb;        //COM device control block
    DCB           dcbc;       //DCB copy
    COMMTIMEOUTS  ComTo;      //COM timeouts
    COMMTIMEOUTS  ComToc;     //COM timeouts copy
    DWORD errors;
    COMSTAT status;
};

#endif // SERIALPORT_WIN_H
