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

#include "wsp32.h"
#include <iostream>

using namespace Mcudrv;

namespace Wk {

static inline const char* getErrorString(uint8_t err) {
    return getErrorString(static_cast<Err>(err));
}
    //--------------------------- Receive frame: --------------------------------

    bool Wake::RxFrame(uint32_t To, uint8_t &ADD, uint8_t &CMD, uint8_t &N, uint8_t* Data)
    {
        using namespace std;
        int i;
        unsigned char b = 0;
        Crc::Crc8 crc(CRC_INIT);          //init CRC
        port_.SetTimeout(To);
#ifdef DEBUG_MODE
        debugInfo_.timeoutSuccess = true;
#endif
        for(i = 0; i < 512 && b != FEND; i++) {
            if(!port_.ReadByte(b))     break;                //frame synchronization
        }
        if(b != FEND) {
            return 0;             //timeout or sync error
        }
#ifdef DEBUG_MODE
        else {
            debugInfo_.syncSuccess = true;
        }
        debugInfo_.staffingSuccess = true;
#endif
        crc(b);                               //update CRC
        N = ADD = 0;
        for(i = -3; i <= N; i++) {
            if(!port_.ReadByte(b)) {
#ifdef DEBUG_MODE
                debugInfo_.syncSuccess = false;
#endif
                break;                //timeout error
            }
            if(b == FESC) {
                if(!port_.ReadByte(b)) {
#ifdef DEBUG_MODE
                    debugInfo_.syncSuccess = false;
#endif
                    break;                //timeout error
                }
                else {
                    if(b == TFEND) {
                        b = FEND;              //TFEND <- FEND
                    }
                    else if(b == TFESC) {
                        b = FESC;              //TFESC <- FESC
                    }
                    else {
#ifdef DEBUG_MODE
                        debugInfo_.staffingSuccess = false;
#endif
                        break;
                    }
                }
            }
            if(i == -3) {
                if(b & 0x80) {
                    ADD = b & 0x7F;	//ADD (b.7=1)
                }
                else {
                    CMD = b;				//CMD (b.7=0)
                    i++;
                }
            }
            else if(i == -2) {
                if(b & 0x80) {
                    break;        //CMD error (b.7=1)
                }
                else {
                    CMD = b;      //CMD
                }
            }
            else if(i == -1) {
                N = b;          //N
            }
            else if(i < N) {
                Data[i] = b;    //data
            }
            else {//if(i == N)
                RxCrc_ = crc.GetResult();
            }
            crc(b);           //update CRC
        }
#ifdef DEBUG_MODE
        debugInfo_.crcSuccess = !crc.GetResult();
#endif
        return ((i == N + 1) && !crc.GetResult());                    //RX or CRC error
    }

    //--------------------------- Transmit frame: -------------------------------

    bool Wake::TxFrame(uint8_t ADDR, uint8_t CMD, uint8_t N, uint8_t* Data)
    {
        unsigned char Buff[280];
        uint32_t j = 0;
        unsigned char d;
        Crc::Crc8 crc(CRC_INIT);
        for(int i = -4; i <= N; ++i) {
            if(i == -3 && !ADDR) {
                ++i;
            }
            if(i == -4) {
                d = FEND;
            }
            else if(i == -3) {	//FEND
                d = ADDR | 0x80;
            }
            else if(i == -2) {	//address
                d = CMD;
            }
            else if(i == -1) {	//command
                d = N;
            }
            else if(i == N) {	//N
                TxCrc_ = d = crc.GetResult();
            }
            else {
                d = Data[i];  //data
            }
            crc(d);
            if(i > -4) {
                if(d == FEND || d == FESC) {
                    Buff[j++] = FESC;
                    d = (d == FEND ? TFEND : TFESC);
                }
            }
            Buff[j++] = d;
        }
        return port_.WriteData(Buff, j);
    }

    bool Wake::GetInfo(Packet_t& packet)
    {
        packet.cmd = C_GETINFO;
        packet.n = 0; //common request
        if(!Request(packet, 50)) {
            std::cerr << ">>> Common Info request failed (maybe bootloader already running)\r\n";
            return false;
        }
        if(packet.payload[0]) {
            std::cerr << "Common Info request failed with device response: "
                      << getErrorString(packet.payload[0]) << endl;
            return false;
        }
        auto data = &packet.payload[1];
        cout << ">>> User Firmware Information\r\n";
        cout << "Protocol Version: " << (static_cast<uint32_t>(data[1] >> 4))
                << '.' << (static_cast<uint32_t>(data[1]) & 0x0F) << "\r\n";
        auto deviceMask = *data;
        cout << "Available modules: \r\n";
        for(size_t i{}; i < DEV_TYPES_NUMBER; ++i) {
            if(deviceMask & (1U << i)) {
                cout << "\t" << deviceTypeStr[i] << "\r\n";
                packet.n = 1;										//device info request
                packet.payload[0] = static_cast<uint8_t>(i); //device select
                if(!Request(packet, 50)) {
                    std::cerr << ">>> Device Info request failed\r\n";
                    continue;
                }
                switch(DeviceType(i))
                {
                case Wk::DEV_LED_DRIVER:
                    cout << "\t\tChannels Number: " << (*data & 0x01 ? 2 : 1) << "\r\n";
                    cout << "\t\tFan Controller present: " << (*data & 0x02 ? "Yes" : "No")
                         << "\r\n";
                    break;
                case Wk::DEV_POWER_SWITCH: case Wk::DEV_RGB_LED_DRIVER:
                    cout << "\t\tChannels Number: " << static_cast<uint32_t>(*data) << "\r\n";
                    break;
            case Wk::DEV_GENERIC_IO:
                    cout << "\t\tMemory area size available: "
                         << static_cast<uint32_t>(*data) << "\r\n";
                    break;
                case Wk::DEV_SENSOR:
                    for(uint8_t i{}; i < sizeof(sensorTypeStr); ++i) {
                        if(*data & (1U << i)) {
                            cout << "\t\tType: " << sensorTypeStr[i] << "\r\n";
                        }
                    }
                    break;
                case Wk::DEV_POWER_SUPPLY:
                    cout << "\t\tNominal Power: ";
                    if(packet.n == 2) {
                        cout << static_cast<uint32_t>(*data) << "W\r\n";
                    }
                    else {
                        cout << *(reinterpret_cast<uint16_t*>(data)) << "W\r\n";
                    }
                    break;
                case Wk::DEV_RESERVED:
                    cout << "\t\tReserved" << "\r\n";
                    break;
                case Wk::DEV_CUSTOM:
                    cout << "\t\tID: " << static_cast<uint32_t>(*data) << "\r\n";
                    break;
                default:
                    break;
                }
            }
        }
        return true;
    }

    const char* GetErrorString(Err err)
    {
        switch(err) {
        case ERR_NO:
            return "OK";
        case ERR_BU:
            return "Device Busy";
        case ERR_RE:
            return "Device Not Ready";
        case ERR_PA:
            return "Command Parameters Is Incorrect";
        case ERR_NI:
            return "Command Not Implemented";
        case ERR_ADDRFMT:
            return "New Address Is Incorrect";
        case ERR_EEPROMUNLOCK:
            return "EEPROM Unlocking Error";
        default:
            return "Unknown Error";
        }
    }

}//Wk
