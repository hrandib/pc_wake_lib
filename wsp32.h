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
#include <windows.h>
//#pragma comment(lib, "Ws2_32.lib")
#undef min
#undef max
#include <stdint.h>
#include <string>
#include <array>
#include <iostream>
#include <iomanip>
#include "wake_lib/crc8.h"

namespace Wk {

#ifdef DEBUG_MODE
	union DebugInfo
	{
		struct
		{
			unsigned txSuccess : 1;
			unsigned rxSuccess : 1;
			unsigned timeoutSuccess : 1;
			unsigned syncSuccess : 1;
			unsigned crcSuccess : 1;
			unsigned staffingSuccess : 1;
		};
		operator bool()
		{
			return data != 0x3F;
		}
		void Print()
		{
			using namespace std;
			cout.setf(cout.boolalpha);
			cout << "TX success: " << (bool)txSuccess << endl;
			cout << "RX success: " << (bool)rxSuccess << endl;
			cout << "Timeout success: " << (bool)timeoutSuccess << endl;
			cout << "Sync success:" << (bool)syncSuccess << endl;
			cout << "CRC success: " << (bool)crcSuccess << endl;
			cout << "Staffing success: " << (bool)staffingSuccess << endl;
			cout.unsetf(cout.boolalpha);
		}
	private:
		uint32_t data;
	};
#endif

	enum Cmd
	{
		C_NOP,    //нет операции
		C_ERR,    //ошибка приема пакета
		C_ECHO,    //передать эхо
		C_GETINFO,
		C_SETNODEADDRESS,
		C_SETGROUPADDRESS,
		C_GETOPTIME,
		C_OFF,
		C_ON,
		C_TOGGLE_ONOFF,
		C_SAVESETTINGS,
		C_REBOOT
	};
	enum Err
	{
		ERR_NO,	//no error
		ERR_TX,	//Rx/Tx error
		ERR_BU,	//device busy error
		ERR_RE,	//device not ready error
		ERR_PA,	//parameters value error
		ERR_NI,	//Command not impl
		ERR_NR,	//no replay
		ERR_NC,	//no carrier
		ERR_ADDRFMT,	//new address is wrong
		ERR_EEPROMUNLOCK //EEPROM wasn't unlocked
	};
	enum DeviceType {
		DEV_LED_DRIVER,
		DEV_POWER_SWITCH,
		DEV_RGB_LED_DRIVER,
		DEV_GENERIC_IO,
		DEV_SENSOR,
		DEV_POWER_SUPPLY,
		DEV_RESERVED,
		DEV_CUSTOM,
		DEV_TYPES_NUMBER
	};
	constexpr static std::array<const char*, DEV_TYPES_NUMBER> deviceTypeStr = {"LED Driver", "Power Switch",
																															"RGB LED Driver", "Generic IO",
																															"Sensor", "Power Supply",
																															"Reserved", "Custom Device"};
	enum SensorType {
		SEN_TEMPERATURE,
		SEN_HUMIDITY,
		SEN_PRESSURE,
		SEN_LIGHT,
		SEN_CO2,
		SEN_PRESENCE,
		SEN_WATERLEAK,
		SEN_TYPES_NUMBER
	};
	constexpr static std::array<const char*, SEN_TYPES_NUMBER> sensorTypeStr = {"Temperature", "Humidity",
																																						"Pressure", "Light",
																																						"CO2", "Presence",
																																						"Water Leak"};
	
	struct Packet_t
	{
		using Self = Packet_t;
		Packet_t() = default;
		Packet_t(Packet_t&& packet) = default;
		Packet_t(const Packet_t& packet) = default;
		Self& operator=(const Packet_t&) = default;
		Packet_t(uint8_t address, uint8_t command, std::initializer_list<uint8_t> data) : addr{address}, cmd{command}, n{(uint8_t)data.size()}
		{
			std::copy(data.begin(), data.end(), payload.begin());
		}
		uint8_t addr;
		uint8_t cmd;
		uint8_t n;
		std::array<uint8_t, 160> payload;
	};
	
	using std::string;
	using std::cout;
	using std::endl;

	const char* GetErrorString(Err err);
	
	class Wake
	{
	private:
		uint8_t TxCrc_, RxCrc_;
		const string portName_;
		DWORD baud_;
#ifdef DEBUG_MODE
		DebugInfo debugInfo_{};
#endif
	public:
		Wake(const char *portName, const DWORD baud) : portName_("//./" + string(portName)), baud_(baud), TxCrc_(0), RxCrc_(0)
		{
			//		connected = OpenCOM(portName, baud);
		}
		Wake(const string& portName, const DWORD baud) : portName_("//./" + portName), baud_(baud), TxCrc_(0), RxCrc_(0)
		{
			//		std::string temp = "//./" + portName;
			//		connected = OpenCOM(temp.c_str(), baud);
		}
		bool IsConnected() const
		{
			return connected;
		}
		bool OpenConnection()
		{
			connected = OpenCOM(portName_.c_str(), baud_);
			return connected;
		}
		bool GetInfo(Packet_t& packet);
		bool RxFrame(Packet_t& packet, uint32_t To)
		{
			return RxFrame(To, packet.addr, packet.cmd, packet.n, packet.payload.data());
		}
		bool TxFrame(Packet_t& packet)
		{
			return TxFrame(packet.addr, packet.cmd, packet.n, packet.payload.data());
		}
#ifndef DEBUG_MODE
		bool Request(Packet_t& packet, uint32_t To = 50)
		{
			if(TxFrame(packet) && RxFrame(packet, To))
			{
				return true;
			}
			return false;
		}
#else //DEBUG_MODE
		auto Request(Packet_t &packet, uint32_t To)
		{
			if(TxFrame(packet))
			{
				debugInfo_.txSuccess = true;
				if(!packet.addr)
				{
					debugInfo_.rxSuccess = true;
					return debugInfo_;
				}
				if(RxFrame(packet, To))
				{
					debugInfo_.rxSuccess = true;
				}
			}
			return debugInfo_;
		}
#endif
		uint8_t GetTxCrc()
		{
			return TxCrc_;
		}
		uint8_t GetRxCrc()
		{
			return RxCrc_;
		}
		~Wake()
		{
			CloseCOM();
		}
	protected:
		bool __fastcall SendData(const uint8_t* data, uint32_t size);
		bool __fastcall SendByte(uint8_t data)
		{
			return SendData(&data, 1);
		}
		bool __fastcall ReceiveByte(unsigned char &b);
	private:
		static const uint8_t CRC_Init = 0xDE;     //CRC Initial value
		HANDLE        hCom;       //COM handle
		DCB           dcb;        //COM device control block
		DCB           dcbc;       //DCB copy
		COMMTIMEOUTS  ComTo;      //COM timeouts
		COMMTIMEOUTS  ComToc;     //COM timeouts copy
		static const unsigned char
			FEND = 0xC0,            //Frame END
			FESC = 0xDB,            //Frame ESCape
			TFEND = 0xDC,            //Transposed Frame END
			TFESC = 0xDD;            //Transposed Frame ESCape
		bool connected;
		DWORD errors;
		COMSTAT status;


		bool OpenCOM(const char *portName, DWORD baud);
		bool CloseCOM(void);

		bool SetModLns(DWORD F);
		bool GetModLns(LPDWORD lpD);
		bool PurgeCOM(void);
		bool FlushCOM(void);
		bool GetMaskCOM(LPDWORD lpEvtMask);
		bool SetMaskCOM(DWORD EvtMask);
		bool WaitEventCOM(LPDWORD lpEvtMask);
		bool AccessCOM(char *P);

		bool RxFrame(DWORD To, unsigned char &ADD,
			unsigned char &CMD, unsigned char &N, unsigned char *Data);

		bool TxFrame(unsigned char ADDR, unsigned char CMD,
			unsigned char N, unsigned char *Data);
	};

}//Wk




 