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

#ifndef UNICODE  
  typedef std::string tstring; 
  typedef std::regex tregex;
  typedef std::smatch tsmatch;
#else
  typedef std::wstring tstring; 
  typedef std::wregex tregex;
  typedef std::wsmatch tsmatch;
#endif

namespace Globals
{
	//Constants
	//
	static const int  PID_APPLE_ALU_WIRELESS_ANSI = 0x022c;
	static const int  PID_APPLE_ALU_WIRELESS_ISO = 0x022d;
	static const int  PID_APPLE_ALU_WIRELESS_JIS = 0x022e;

	static const int PID_APPLE_ALU_WIRELESS_2009_ANSI = 0x0239;
	static const int PID_APPLE_ALU_WIRELESS_2009_ISO = 0x023a;
	static const int PID_APPLE_ALU_WIRELESS_2009_JIS = 0x023b;

	static const int PID_APPLE_ALU_WIRELESS_2011_ANSI = 0x0255;
	static const int PID_APPLE_ALU_WIRELESS_2011_ISO = 0x0256;

	static const int VID_APPLE = 0x5ac;

	//Global functions
	//
	extern void DebugMsg(const TCHAR* szFormat, ...);
	extern tstring GetLastErrorStr(DWORD dwErrorCode);
	extern bool GetRegKeyDwordValue(const HKEY& hKey, const tstring& location, const tstring& name, DWORD& value);
	extern void Tokenize(const tstring& str, vector<tstring>& tokens, const tstring& delimiters = _T(" "));
}

