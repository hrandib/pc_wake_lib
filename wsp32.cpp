#include "wsp32.h"
#include <iostream>

using namespace Mcudrv;

//----------------------- Access check for COM resource: --------------------

bool Wake::AccessCOM(char *P)
{ 
	HANDLE hTemp = CreateFile(P,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);
 if (hTemp != INVALID_HANDLE_VALUE)
  { CloseHandle(hTemp); return 1; }
  else return 0;
}

//----------------- Open COM for non overlapped operations: -----------------

bool Wake::OpenCOM(const char *P, DWORD baud)
{
 hCom = CreateFile(P,
                   GENERIC_READ | GENERIC_WRITE,
                   0,
                   NULL,
                   OPEN_EXISTING,
                   0,
                   NULL);
 if (hCom == INVALID_HANDLE_VALUE)    return 0; //port open error
 if (!GetCommState(hCom, &dcb))       return 0; //get state error
 if (!GetCommTimeouts(hCom, &ComTo))  return 0; //get timeouts error
 dcbc = dcb; ComToc = ComTo;                    //save dcb and timeouts
 dcb.BaudRate = baud;                           //set baud rate
 dcb.ByteSize = 8;                              //set byte size
 dcb.Parity = NOPARITY;                         //set no parity
 dcb.StopBits = ONESTOPBIT;                     //set one stop bit
 dcb.fBinary = 1;                               //binary mode
 dcb.fDtrControl = DTR_CONTROL_DISABLE;         //clear DTR
 dcb.fRtsControl = RTS_CONTROL_DISABLE;          //clear RTS
 if (!SetCommState(hCom, &dcb))       return 0; //set state error
 if (!SetupComm(hCom, 512, 512))      return 0; //setup error
 ComTo.ReadIntervalTimeout = 0;		//Not Used
 ComTo.ReadTotalTimeoutMultiplier = 2;
 ComTo.ReadTotalTimeoutConstant = 1000;
 ComTo.WriteTotalTimeoutMultiplier = 0;
 ComTo.WriteTotalTimeoutConstant = 1000;
 if (!SetCommTimeouts(hCom, &ComTo))  return 0; //set timeouts error
 if (!PurgeCOM())                     return 0; //purge com error
 return 1;
}

//------------------------------ Close COM: ---------------------------------

bool Wake::CloseCOM(void)
{
 if (!SetCommState(hCom, &dcbc))      return 0; //set state error
 if (!SetCommTimeouts(hCom, &ComToc)) return 0; //set timeouts error
 if (!CloseHandle(hCom))              return 0; //port close error
 return 1;
}

//-------------------------- Set modem lines: -------------------------------

bool Wake::SetModLns(DWORD F)
{
 return EscapeCommFunction(hCom, F) != 0;
}

//-------------------------- Get modem lines: -------------------------------

bool Wake::GetModLns(LPDWORD lpD)
{
 return GetCommModemStatus(hCom, lpD) != 0;
}

//------------- Purge COM: terminates TX and RX and clears buffers: ---------

bool Wake::PurgeCOM(void)
{
 return PurgeComm(hCom, PURGE_TXABORT |
                        PURGE_RXABORT |
                        PURGE_TXCLEAR |
                        PURGE_RXCLEAR) != 0;
}

//--------------- Flush COM: TX buffer RX and clear then return: ------------

bool Wake::FlushCOM(void)
{
 return FlushFileBuffers(hCom) != 0;
}

//------------------------- Get COM events mask: ----------------------------

bool Wake::GetMaskCOM(LPDWORD lpEvtMask)
{
 return GetCommMask(hCom, lpEvtMask) != 0;
}

//------------------------- Set COM events mask: ----------------------------

bool Wake::SetMaskCOM(DWORD EvtMask)
{
 return SetCommMask(hCom, EvtMask) != 0;
}

//--------------------------- Wait COM event: -------------------------------

bool Wake::WaitEventCOM(LPDWORD lpEvtMask)
{
 return WaitCommEvent(hCom, lpEvtMask, NULL) != 0;
}

//---------------------------------------------------------------------------

// void __fastcall Wake::DowCRC(unsigned char b, unsigned char &crc)
// { for (int i = 0; i < 8; i++)
//   {
//    if (((b ^ crc) & 1) != 0)
//      crc = ((crc ^ 0x18) >> 1) | 0x80;
//        else crc = (crc >> 1) & ~0x80;
//    b = b >> 1;
//    }
// }

//---------------------------------------------------------------------------

bool Wake::SendData(const uint8_t* data, uint32_t size)
{
	DWORD written;
	bool x = WriteFile(hCom, data, size, &written, NULL) != 0;
	return x && written == size;
}

bool Wake::ReceiveByte(unsigned char &b)
{
	DWORD read; bool x;
	x = ReadFile(hCom, &b, 1, &read, NULL) != 0;         //read byte
	return (x && (read == 1));
}

//--------------------------- Receive frame: --------------------------------

bool Wake::RxFrame(DWORD To, unsigned char &ADD, unsigned char &CMD,
                                      unsigned char &N, unsigned char *Data)
{
	using namespace std;
	int i;
	unsigned char b = 0;
	Crc::Crc8 crc(CRC_Init);          //init CRC
	ComTo.ReadTotalTimeoutConstant = To;          //set timeout
	if(!SetCommTimeouts(hCom, &ComTo)) {
		return 0; //set timeouts error
	}
	debugInfo_.timeoutSuccess = true;
	for(i = 0; i < 512 && b != FEND; i++) {
		if(!ReceiveByte(b))     break;                //frame synchronization
	}
	if(b != FEND) {
		return 0;             //timeout or sync error
	}
	else {
		debugInfo_.syncSuccess = true;
	}
	debugInfo_.staffingSuccess = true;
	crc(b);                               //update CRC
	N = ADD = 0;
	for(i = -3; i <= N; i++) {
		if(!ReceiveByte(b)) {
			debugInfo_.syncSuccess = false;
			break;                //timeout error
		}
		if(b == FESC) {
			if(!ReceiveByte(b)) {
				debugInfo_.syncSuccess = false;
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
					debugInfo_.staffingSuccess = false;
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
		debugInfo_.crcSuccess = !crc.GetResult();
		return ((i == N + 1) && !crc.GetResult());                    //RX or CRC error
}

//--------------------------- Transmit frame: -------------------------------

bool Wake::TxFrame(unsigned char ADDR, unsigned char CMD,
					unsigned char N, unsigned char *Data)
{
	unsigned char Buff[280];
	DWORD j = 0;
	unsigned char d;
	Crc::Crc8 crc(CRC_Init);
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
//	bool x = WriteFile(hCom, Buff, j, &r, NULL) != 0;  //TX frame
	return SendData(Buff, j);
}
