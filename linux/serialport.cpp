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

#include "serialport.h"

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <stdexcept>

SerialPort::SerialPort(stringv portPath, uint32_t baudRate)
    : portName_{portPath}, baudConstant_{GetBaudConstant(baudRate)}, fd_{}
{  }

bool SerialPort::AccessCOM() {
    return false;
}

bool SerialPort::OpenCOM()
{
    bool result = false;
    if(fd_ == 0) {
        fd_ = open(portName_.data(), O_RDWR | O_NOCTTY);
    }
    if(fd_ > 0) {
        result = true;
    }
    return result && SetPortAttributes();
}

bool SerialPort::CloseCOM()
{
    bool result = !close(fd_);
    if(result) {
        fd_ = 0;
    }
    return result;
}

bool SerialPort::SetTimeout(uint32_t) {
    return true;
}

SerialPort::~SerialPort()
{
    if(fd_ > 0) {
        close(fd_);
    }
}

bool SerialPort::SetPortAttributes() {
    termios config;
    if(tcgetattr(fd_, &config) < 0) {
        return false;
    }

    cfmakeraw(&config);

    //
    // 300ms timeout for the packet
    // Inter-character timer off
    //
    config.c_cc[VMIN]  = 0;
    config.c_cc[VTIME] = 3;

    //
    // Communication speed (simple version, using the predefined
    // constants)
    //
    if(cfsetspeed(&config, baudConstant_) < 0) {
        return false;
    }

    //
    // Finally, apply the configuration
    //
    if(tcsetattr(fd_, TCSAFLUSH, &config) < 0) {
        return false;
    }
    return true;
}

uint32_t SerialPort::GetBaudConstant(uint32_t baudRate)
{
    switch(baudRate) {
    case 50:    return 0000001;
    case 75:    return 0000002;
    case 110:   return 0000003;
    case 134:   return 0000004;
    case 150:   return 0000005;
    case 200:   return 0000006;
    case 300:   return 0000007;
    case 600:   return 0000010;
    case 1200:  return 0000011;
    case 1800:  return 0000012;
    case 2400:  return 0000013;
    case 4800:  return 0000014;
    case 9600:  return 0000015;
    case 19200: return 0000016;
    case 38400: return 0000017;
    case 57600: return 0010001;
    case 115200: return 0010002;
    case 230400: return 0010003;
    case 460800: return 0010004;
    case 500000: return 0010005;
    case 576000: return 0010006;
    case 921600: return 0010007;
    case 1000000: return 0010010;
    case 1152000: return 0010011;
    case 1500000: return 0010012;
    case 2000000: return 0010013;
    case 2500000: return 0010014;
    case 3000000: return 0010015;
    case 3500000: return 0010016;
    case 4000000: return 0010017;
    default:
        throw std::invalid_argument("baudRate is not in standard range" + std::to_string(baudRate));
    }
}

bool SerialPort::WriteData(const uint8_t* data, uint32_t size)
{
    return write(fd_, data, size) > 0;
}

bool SerialPort::ReadData(uint8_t* data, uint32_t size)
{
    return read(fd_, data, size) > 0;
}

bool SerialPort::ResetStatus() {
    return true;
}

bool SerialPort::Flush() {
    return true;
}
