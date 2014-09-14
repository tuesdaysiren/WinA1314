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

#pragma once

class CInterceptor
{
public:
	typedef unsigned short InterceptionFilter;

	enum InterceptionKeyState
	{
		INTERCEPTION_KEY_DOWN             = 0x00,
		INTERCEPTION_KEY_UP               = 0x01,
		INTERCEPTION_KEY_E0               = 0x02,
		INTERCEPTION_KEY_E1               = 0x04,
		INTERCEPTION_KEY_TERMSRV_SET_LED  = 0x08,
		INTERCEPTION_KEY_TERMSRV_SHADOW   = 0x10,
		INTERCEPTION_KEY_TERMSRV_VKPACKET = 0x20
	};


	enum InterceptionFilterKeyState
	{
		INTERCEPTION_FILTER_KEY_NONE             = 0x0000,
		INTERCEPTION_FILTER_KEY_ALL              = 0xFFFF,
		INTERCEPTION_FILTER_KEY_DOWN             = INTERCEPTION_KEY_UP,
		INTERCEPTION_FILTER_KEY_UP               = INTERCEPTION_KEY_UP << 1,
		INTERCEPTION_FILTER_KEY_E0               = INTERCEPTION_KEY_E0 << 1,
		INTERCEPTION_FILTER_KEY_E1               = INTERCEPTION_KEY_E1 << 1,
		INTERCEPTION_FILTER_KEY_TERMSRV_SET_LED  = INTERCEPTION_KEY_TERMSRV_SET_LED << 1,
		INTERCEPTION_FILTER_KEY_TERMSRV_SHADOW   = INTERCEPTION_KEY_TERMSRV_SHADOW << 1,
		INTERCEPTION_FILTER_KEY_TERMSRV_VKPACKET = INTERCEPTION_KEY_TERMSRV_VKPACKET << 1
	};

	typedef struct InterceptionKeyStroke
	{
		unsigned short code;
		unsigned short state;
		unsigned int information;
	};

	typedef struct _KEYBOARD_INPUT_DATA
	{
		USHORT UnitId;
		USHORT MakeCode;
		USHORT Flags;
		USHORT Reserved;
		ULONG  ExtraInformation;
	} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

public:
	HANDLE m_hEvent;

public:
	CInterceptor(void);
	virtual ~CInterceptor(void);

	bool Open();
	void Close();
	bool IsOpen() { return m_hDevice != NULL; }

	int Read(InterceptionKeyStroke* stroke);
	int Write(const InterceptionKeyStroke& stroke);

protected:
	HANDLE m_hDevice;

protected:
	bool MatchHwString(int nVid, int nPid, const wchar_t* szHwStr);
};


