//////////////////////////////////////////////////////////////////////////////////////
//
// WinA1314 Apple Wireless Keyboard driver for Windows.
//
// Copyright (c) George Samartzidis <samartzidis@gmail.com>.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////////////////
//

#include "stdafx.h"
#include "Interceptor.h"
#include "Globals.h"

#define IOCTL_SET_PRECEDENCE    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_PRECEDENCE    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_FILTER        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_FILTER        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_EVENT         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x810, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE             CTL_CODE(FILE_DEVICE_UNKNOWN, 0x820, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ              CTL_CODE(FILE_DEVICE_UNKNOWN, 0x840, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_HARDWARE_ID   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x880, METHOD_BUFFERED, FILE_ANY_ACCESS)

CInterceptor::CInterceptor(void)
{
	m_hDevice = NULL;
	m_hEvent = NULL;
}

CInterceptor::~CInterceptor(void)
{
	Close();
}

void CInterceptor::Close()
{
	if(m_hEvent)
	{
		SetEvent(m_hEvent);
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}

	if(m_hDevice)
	{
		CloseHandle(m_hDevice);	
		m_hDevice = NULL;
	}
}

bool CInterceptor::Open()
{
	if(IsOpen())
	{
		Globals::DebugMsg(_T("CInterceptor::Open(): Already initialised.\n"));
		return true;
	}

	/*
	tregex ex(_T("VID_(\\w{4})&PID_(\\w{4})&Col(\\w{2})|VID&\\w{4}(\\w{4})_PID&(\\w{4})&Col(\\w{2})"), tregex::ECMAScript | tregex::icase );
	for(int k = 0; k < 10; k++)
	{
		TCHAR szAlias[32];
		_stprintf_s(szAlias, _T("\\\\.\\interception0%d"), k);
		m_hDevice = CreateFile(szAlias, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

		wchar_t szHwId[1024];
		ZeroMemory(szHwId, sizeof(szHwId));
		DWORD dwOutSize = 0;
		BOOL bRes = DeviceIoControl(m_hDevice, IOCTL_GET_HARDWARE_ID, NULL, 0, szHwId, sizeof(szHwId), &dwOutSize, NULL);
		if(bRes)
		{
			Globals::DebugMsg(_T("CInterceptor::Open(): %s : %s\n"), szAlias, szHwId);

			//Match regex against hardware id
			tsmatch match;
			tstring strHwId = tstring(szHwId);
			regex_search(strHwId, match, ex);
			int nVid = 0; int nPid = 0; int nCol = 0;

			if(match.size() >= 6)
			{
				if(match[1].matched)
				{
					nVid = _tcstol(match[1].str().c_str(), NULL, 16);
					nPid = _tcstol(match[2].str().c_str(), NULL, 16);
					nCol = _tcstol(match[3].str().c_str(), NULL, 16);
				}
				else
				{
					nVid = _tcstol(match[4].str().c_str(), NULL, 16);
					nPid = _tcstol(match[5].str().c_str(), NULL, 16);
					nCol = _tcstol(match[6].str().c_str(), NULL, 16);
				}
			}

			if(
				nVid == Globals::VID_APPLE 
				&& 
				(
					nPid == Globals::PID_APPLE_ALU_WIRELESS_2009_ANSI ||
					nPid == Globals::PID_APPLE_ALU_WIRELESS_2009_ISO  ||
					nPid == Globals::PID_APPLE_ALU_WIRELESS_2009_JIS  ||
					nPid == Globals::PID_APPLE_ALU_WIRELESS_2011_ANSI ||
					nPid == Globals::PID_APPLE_ALU_WIRELESS_2011_ISO)
				)
			{
				Globals::DebugMsg(_T("CInterceptor::Open(): TLC 01 interface matched: %s : %s\n"), szAlias, szHwId);
				break;
			}
		}

		CloseHandle(m_hDevice);
		m_hDevice = NULL;
	}
	*/	

	for(int k = 0; k < 10; k++)
	{
		TCHAR szAlias[32];
		_stprintf_s(szAlias, _T("\\\\.\\interception0%d"), k);
		m_hDevice = CreateFile(szAlias, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

		wchar_t szHwId[1024];
		ZeroMemory(szHwId, sizeof(szHwId));
		DWORD dwOutSize = 0;
		BOOL bRes = DeviceIoControl(m_hDevice, IOCTL_GET_HARDWARE_ID, NULL, 0, szHwId, sizeof(szHwId), &dwOutSize, NULL);
		if(bRes)
		{
			Globals::DebugMsg(_T("CInterceptor::Open(): %s : %s\n"), szAlias, szHwId);
		}

		if(MatchHwString(Globals::VID_APPLE, Globals::PID_APPLE_ALU_WIRELESS_ANSI, szHwId) ||
			MatchHwString(Globals::VID_APPLE, Globals::PID_APPLE_ALU_WIRELESS_ISO, szHwId) ||
			MatchHwString(Globals::VID_APPLE, Globals::PID_APPLE_ALU_WIRELESS_JIS, szHwId) ||
			MatchHwString(Globals::VID_APPLE, Globals::PID_APPLE_ALU_WIRELESS_2009_ANSI, szHwId) ||
			MatchHwString(Globals::VID_APPLE, Globals::PID_APPLE_ALU_WIRELESS_2009_ISO, szHwId) ||
			MatchHwString(Globals::VID_APPLE, Globals::PID_APPLE_ALU_WIRELESS_2009_JIS, szHwId) ||
			MatchHwString(Globals::VID_APPLE, Globals::PID_APPLE_ALU_WIRELESS_2011_ANSI, szHwId) ||
			MatchHwString(Globals::VID_APPLE, Globals::PID_APPLE_ALU_WIRELESS_2011_ISO, szHwId))
			break;

		CloseHandle(m_hDevice);
		m_hDevice = NULL;
	}

	if(m_hDevice == NULL)
	{
		Globals::DebugMsg(_T("A1314 keyboard not found\n"));
		return false;
	}

	m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	HANDLE hZeroPaddedHandle[2] = {m_hEvent, 0};
	DWORD dwBytesReturned = 0;
	if(!DeviceIoControl(m_hDevice, IOCTL_SET_EVENT, hZeroPaddedHandle, sizeof(hZeroPaddedHandle), 
		NULL, 0, &dwBytesReturned, NULL))
    {
		Globals::DebugMsg(_T("Failed to set event for device\n"));
		CloseHandle(m_hDevice);
		CloseHandle(m_hEvent);
		m_hDevice = NULL;
		m_hEvent = NULL;

		return false;
    }

	InterceptionFilter filter = INTERCEPTION_FILTER_KEY_ALL;
	DeviceIoControl(m_hDevice, IOCTL_SET_FILTER, (LPVOID)&filter, sizeof(InterceptionFilter), 
		NULL, 0, &dwBytesReturned, NULL);

	return true;
}

bool CInterceptor::MatchHwString(int nVid, int nPid, const wchar_t* szHwStr)
{
	TCHAR szVid[8], szPid[8], szHwStrLower[1024];

	_tcscpy_s(szHwStrLower, szHwStr);
	for(int i = 0; szHwStrLower[i]; i++) { szHwStrLower[i] = tolower(szHwStrLower[i]); }

	_stprintf_s(szVid, _T("%04x"), nVid);
	_stprintf_s(szPid, _T("%04x"), nPid);

	return (_tcsstr(szHwStrLower, _T("hid")) != 0 && _tcsstr(szHwStrLower, szVid) != 0 && _tcsstr(szHwStrLower, szPid) != 0);
}

int CInterceptor::Read(InterceptionKeyStroke* stroke)
{
	PKEYBOARD_INPUT_DATA rawstroke = (PKEYBOARD_INPUT_DATA)HeapAlloc(GetProcessHeap(), 0, sizeof(KEYBOARD_INPUT_DATA));

	DWORD bytesread = 0;
	DeviceIoControl(m_hDevice, IOCTL_READ, NULL, 0, rawstroke, sizeof(KEYBOARD_INPUT_DATA), &bytesread, NULL);
	int strokesRead = bytesread / sizeof(KEYBOARD_INPUT_DATA);

	if(strokesRead > 0)
	{
		stroke->code = rawstroke->MakeCode;
		stroke->state = rawstroke->Flags;
		stroke->information = rawstroke->ExtraInformation;
	}

	HeapFree(GetProcessHeap(), 0,  rawstroke);

	return strokesRead;
}

int CInterceptor::Write(const InterceptionKeyStroke& stroke)
{
	PKEYBOARD_INPUT_DATA rawstroke = (PKEYBOARD_INPUT_DATA)HeapAlloc(GetProcessHeap(), 0, sizeof(KEYBOARD_INPUT_DATA));

	rawstroke->UnitId = 0;
	rawstroke->MakeCode = stroke.code;
	rawstroke->Flags = stroke.state;
	rawstroke->Reserved = 0;
	rawstroke->ExtraInformation = stroke.information;

	DWORD dwDataWritten = 0;
	DeviceIoControl(m_hDevice, IOCTL_WRITE, rawstroke, (DWORD)sizeof(KEYBOARD_INPUT_DATA), NULL, 0, &dwDataWritten, NULL);

	HeapFree(GetProcessHeap(), 0,  rawstroke);

	return dwDataWritten / sizeof(KEYBOARD_INPUT_DATA);
}