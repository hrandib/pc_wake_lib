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
#include <fstream>
#include "wake_lib/crc8.h"
#include "crc32.h"

using std::string;
using std::cout;
using std::endl;

using namespace Crc32_CompileTime;
//using namespace WkBoot;

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

class Wake
{
public:
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
private:
	uint8_t TxCrc_, RxCrc_;
	const string portName_;
	DWORD baud_;
	DebugInfo debugInfo_{ };
public:
	enum Cmd
	{
		C_NOP,    //нет операции
		C_ERR,    //ошибка приема пакета
		C_ECHO,    //передать эхо
		C_GETINFO,
		C_SETNODEADDRESS,
		C_SETGROUPADDRESS,
		C_SAVESETTINGS,
		C_GETOPTIME,
		C_OFF,
		C_ON
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
	Wake(const char *portName, const DWORD baud) : portName_("//./" + string(portName)), baud_(baud), TxCrc_(0), RxCrc_(0)
	{
		//		connected = OpenCOM(portName, baud);
	}
	Wake(const string& portName, const DWORD baud) : portName_("//./" + portName), baud_(baud), TxCrc_(0), RxCrc_(0)
	{
		//		std::string temp = "//./" + portName;
		//		connected = OpenCOM(temp.c_str(), baud);
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
	bool IsConnected() const
	{
		return connected;
	}
	bool OpenConnection()
	{
		connected = OpenCOM(portName_.c_str(), baud_);
		return connected;
	}
	bool RxFrame(Packet_t &packet, uint32_t To)
	{
		return RxFrame(To, packet.addr, packet.cmd, packet.n, packet.payload.data());
	}
	bool TxFrame(Packet_t &packet)
	{
		return TxFrame(packet.addr, packet.cmd, packet.n, packet.payload.data());
	}
	bool Request(Packet_t &packet, uint32_t To)
	{
		if(TxFrame(packet) && RxFrame(packet, To))
		{
			return true;
		}
		return false;
	}
	auto DebugRequest(Packet_t &packet, uint32_t To)
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
		FEND  = 0xC0,            //Frame END
		FESC  = 0xDB,            //Frame ESCape
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
#if 0
class WakeBoot : public Wake
{
private:
	enum InstructionSet {
		C_NOP,
		C_ERR,
		C_ECHO,
		C_GETINFO,
		C_REBOOT = 11,
		C_SETPOSITION,
		C_READ,
		C_WRITE,
		C_GO
	};
	OptionsParser& parser_;
	Packet_t params_;
	bool PrintDevicesInfo()
	{
		constexpr static std::array<const char*, 8> deviceClasses = {	"LED Driver", "Power Switch",
																																"RGB LED Driver", "Generic IO",
																																"Sensor", "Power Supply"};
		params_.cmd = C_GETINFO;
		params_.addr = parser_.GetAddress();
		params_.n = 0;
		if(!Request(params_, 50)) {
			cout << "Get device info not succeed\r\n";
			return false;
		}
		for(size_t i{}; i < deviceClasses.size(); ++i) {
			auto availableDevicesMask = params_.payload[0];
			if(availableDevicesMask & (1U << i)) {
				cout << deviceClasses[i] << "\r\n";
			}
		}
		return true;
	}
	bool SendRebootCommand()
	{
		using namespace Utils;
		constexpr static uint32_t REBOOT_KEY = htonl(0xCB47ED91);
		params_.cmd = C_REBOOT;
		params_.addr = parser_.GetAddress();
		params_.n = 4;
		*(uint32_t*)&params_.payload[0] = REBOOT_KEY;
		Request(params_, 50);
		return true;
	}
public:
	WakeBoot(OptionsParser& parser) : Wake(parser.GetPort(), parser.GetBaudRate()),
																		parser_(parser), params_{BOOTADDRESS, C_ECHO, {BOOTSTART_KEY}}
	{ }
	bool OpenConnection()
	{
		uint8_t data;
		if(Wake::OpenConnection()) {
			for(size_t i = 0; i < 512; i++) {
				if(!ReceiveByte(data)) break;			 //frame synchronization
			}
//			PrintDevicesInfo();
//			SendRebootCommand();
			using namespace std;
			if(SendByte(BOOTSTART_KEY) && ReceiveByte(data) && data == BOOTRESPONSE) {
				cout << "Handshake successful" << endl;
				return true;
			}
			else {
				if(Request(params_, 50) && params_.payload[0] == BOOTSTART_KEY) {
					cout << "Handshake already succeed" << endl;
					return true;
				}
			}
			cout << "Handshake not succeed" << endl;
		}
		return false;
	}
	void GetEcho()
	{
		using namespace std;
		params_.cmd = C_ECHO;
		auto intlist = parser_.GetEchoPayload();
		if(intlist.size() > 128) {
			intlist.resize(128);
		}
		params_.n = (uint8_t)intlist.size();
		std::copy(intlist.begin(), intlist.end(), params_.payload.begin());
		if(Request(params_, 50)) {
			cout << "Echo Response: ";
			for(size_t i{}; i < params_.n; ++i) {
				cout << (uint32_t)params_.payload[i] << ' ';
			}
			cout << endl;
		}
		else {
			cout << "Echo Request Failed" << endl;
		}
	}
	const char* GetMcuModel(McuId id)
	{
		switch(id) {
		case ID_STM8S003F3:
			return "STM8S003F3";
		case ID_STM8S103F3:
			return "STM8S103F3";
		case ID_STM8L051F3:
			return "STM8L051F3";
		case ID_STM8S105C6:
			return "STM8S105C6";
		default:
			return "Unknown";
		}
	}
	void GetInfo()
	{
		params_.addr = BOOTADDRESS;
		params_.cmd = C_GETINFO;
		params_.payload[0] = BOOTLOADER_KEY;
		params_.n = 1;
		if(Request(params_, 50)) {
			cout << "Request Status: " << GetErrorString((Err)params_.payload[0]) << "\r\n";
			if(!params_.payload[0]) {
				McuId id = McuId(params_.payload[1] >> 4);
				cout << "Device Bootloader Version: " << uint32_t(params_.payload[1] & 0x0F) << "\r\n"
				<<	"MCU Model: " << GetMcuModel(id) << "\r\n"
				<<  "Bootloader Section Size: " << (uint32_t)params_.payload[2] * (id < ID_STM8S105C6 ? 64 : 128) << " Bytes" << endl;
			}
		}
		else {
			cout << "Get Information Command Failed" << endl;
		}
	}
	void PrintPayload(size_t offset = 0)
	{
		using namespace std;
		cout << setfill('0') << "Payload Size: " << (uint32_t)params_.n - offset << "\r\n";
		for(size_t i{}; i < params_.n - offset; i += 2) {
			cout << hex << setw(4) << Utils::ntohs(*(uint16_t*)&params_.payload[i + offset]) << ' ';
			if(!((i + 2) % 16) && i) {
				cout << "\r\n";
			}
		}
		cout << dec << setfill(' ') << endl;
	}
	void PrintPacket()
	{
		cout << "Address: " << (uint32_t)params_.addr << endl
			<< "Command: " << (uint32_t)params_.cmd << endl
			<< "Error: " << (uint32_t)params_.payload[0] << ' ' << GetErrorString((Err)params_.payload[0]) << endl;
		PrintPayload();
	}
	bool WriteFlash()
	{
		GetInfo();
		using namespace std;
		static constexpr size_t WRITE_TIMEOUT = 350;
		string filePath = parser_.GetFilePath();
		ifstream fs{filePath, ios::binary | ios::ate};
		int64_t fsize{};
		if(!fs) {
			cout << "File not opened\r\n";
			return false;
		}
		fsize = fs.tellg();
		cout << "Firmware Size: " << fsize << " Bytes\r\n";
		if(fsize > 32768) {
			cout << "Error: firmware size must not exceed 32 kbytes" << endl;
			return false;
		}
		fs.seekg(0, fs.beg);
		vector<uint8_t> firmware((size_t)fsize);
		fs.read((char*)firmware.data(), fsize);
		if(!fs) {
			cout << "Error: file not read" << endl;
			return false;
		}
		else {
			cout << "File Read" << endl;
		}
		params_.cmd = WkBoot::C_WRITE;
		auto pos = firmware.begin();
		auto remained = firmware.size();
		constexpr static size_t MAX_PACKET_PAYLOAD = 128;
		while(remained > MAX_PACKET_PAYLOAD) {
			params_.n = MAX_PACKET_PAYLOAD;
			copy(pos, pos + MAX_PACKET_PAYLOAD, params_.payload.begin());
			pos += MAX_PACKET_PAYLOAD;
			remained -= MAX_PACKET_PAYLOAD;
			PrintPayload();
			if(Request(params_, WRITE_TIMEOUT)) {
				cout << "\r\nWritten: " << MAX_PACKET_PAYLOAD << " Bytes. Remained: " << remained << " Bytes" << endl;
			}
			else
			{
				cout << "Send Failed" << endl;
				return false;
			}
		}
		//remained <= 128
		params_.n = (uint8_t)remained;
		copy(pos, pos + remained, params_.payload.begin());
		PrintPayload();
		if(Request(params_, WRITE_TIMEOUT)) {
			cout << "\rWritten: " << fsize << " Bytes" << endl;
		}
		else {
			cout << "Send Failed" << endl;
			return false;
		}
		cout << "Download Succeed" << endl;
		return true;
	}
	void Read()
	{
		params_.cmd = C_READ;
		params_.payload[0] = 128;
		params_.n = 1;
		if(Request(params_, 50)) {
			cout << "Request Status: " << GetErrorString((Err)params_.payload[0]) << "\r\n";
			if(!params_.payload[0]) {
				PrintPayload(3);
				uint16_t curPointer = Utils::ntohs(*(uint16_t*)&params_.payload[1]);
				cout << "Pointer position: " << curPointer << "\r\n";
			}
		}
		else {
			cout << "Read Command Failed" << endl;
		}
	}
	void SetPosition(uint16_t pos)
	{
		params_.cmd = C_SETPOSITION;
		*(uint16_t*)&params_.payload[0] = Utils::htons(pos);
		params_.n = 2;
		if(Request(params_, 50)) {
			cout << "Request Status: " << GetErrorString((Err)params_.payload[0]) << "\r\n";
			if(!params_.payload[0]) {
				uint16_t curPointer = Utils::ntohs(*(uint16_t*)&params_.payload[1]);
				cout << "Pointer position: " << std::hex << curPointer << "\r\n";
			}
		}
		else {
			cout << "Set Position Command Failed" << endl;
		}
	}
	void Run()
	{
		params_.cmd = C_GO;
		params_.n = 1;
		params_.payload[0] = BOOTLOADER_KEY;
		if(Request(params_, 50)) {
			PrintPacket();
		}
	}
	void Process()
	{
		switch(parser_.GetCommand())
		{
		case "echo"_crc: 
			GetEcho();
			break;
		case "getinfo"_crc:
			GetInfo();
			break;
		case "read"_crc:
			Read();
			break;
		case "set"_crc:
			SetPosition(parser_.getPosition());
			break;
		case "flash"_crc:
			cout << "Download to Flash" << endl;
			WriteFlash();
			break;
		case "eeprom"_crc:
			cout << "Download to EEPROM" << endl;
			break;
		case "run"_crc:
			cout << "Jump to User Firmware" << endl;
			Run();
			break;
		default:
			cout << "Wrong Command" << endl;
		}//switch

	}
};
#endif





 