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

#include "serialport_win.h"

SerialPort::SerialPort(stringv portPath, uint32_t baud)
    : portName_{portPath}, baud_{baud}
{  }

//----------------------- Access check for COM resource: --------------------

bool SerialPort::AccessCOM()
{
    HANDLE hTemp = CreateFile(portName_.data(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if(hTemp != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hTemp); return 1;
    }
    else return 0;
}

//----------------- Open COM for non overlapped operations: -----------------

bool SerialPort::OpenCOM()
{
    hCom = CreateFile(portName_.data(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if(hCom == INVALID_HANDLE_VALUE)    return 0; //port open error
    if(!GetCommState(hCom, &dcb))       return 0; //get state error
    if(!GetCommTimeouts(hCom, &ComTo))  return 0; //get timeouts error
    dcbc = dcb; ComToc = ComTo;                    //save dcb and timeouts
    dcb.BaudRate = baud_;                           //set baud rate
    dcb.ByteSize = 8;                              //set byte size
    dcb.Parity = NOPARITY;                         //set no parity
    dcb.StopBits = ONESTOPBIT;                     //set one stop bit
    dcb.fBinary = 1;                               //binary mode
    dcb.fDtrControl = DTR_CONTROL_DISABLE;         //clear DTR
    dcb.fRtsControl = RTS_CONTROL_DISABLE;          //clear RTS
    if(!SetCommState(hCom, &dcb))       return 0; //set state error
    if(!SetupComm(hCom, 512, 512))      return 0; //setup error
    ComTo.ReadIntervalTimeout = 0;		//Not Used
    ComTo.ReadTotalTimeoutMultiplier = 2;
    ComTo.ReadTotalTimeoutConstant = 1000;
    ComTo.WriteTotalTimeoutMultiplier = 0;
    ComTo.WriteTotalTimeoutConstant = 1000;
    if(!SetCommTimeouts(hCom, &ComTo))  return 0; //set timeouts error
    if(!ResetStatus())                     return 0; //purge com error
    return 1;
}

//------------------------------ Close COM: ---------------------------------

bool SerialPort::CloseCOM(void)
{
    if(!SetCommState(hCom, &dcbc))      return 0; //set state error
    if(!SetCommTimeouts(hCom, &ComToc)) return 0; //set timeouts error
    if(!CloseHandle(hCom))              return 0; //port close error
    return 1;
}

bool SerialPort::ResetStatus()
{
    return PurgeComm(hCom, PURGE_TXABORT |
        PURGE_RXABORT |
        PURGE_TXCLEAR |
        PURGE_RXCLEAR) != 0;
}

bool SerialPort::Flush()
{
    return FlushFileBuffers(hCom) != 0;
}

bool SerialPort::setTimeout(uint32_t to)
{
    ComTo.ReadTotalTimeoutConstant = to;
    if(!SetCommTimeouts(hCom, &ComTo)) {
        return false; //set timeouts error
    }
    return true;
}

bool SerialPort::SendData(const uint8_t* data, uint32_t size)
{
    DWORD written;
    bool x = WriteFile(hCom, data, size, &written, NULL) != 0;
    return x && written == size;
}

bool SerialPort::ReceiveByte(unsigned char &b)
{
    DWORD read; bool x;
    x = ReadFile(hCom, &b, 1, &read, NULL) != 0;         //read byte
    return (x && (read == 1));
}
