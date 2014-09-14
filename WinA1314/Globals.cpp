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
#include "Globals.h"
#include <strsafe.h>

namespace Globals
{	
	static inline bool IsInDiagnosticMode()
	{
		static DWORD dwVal = 0xffffffff;
		if(dwVal == 0xffffffff)
			Globals::GetRegKeyDwordValue(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\WinA1314"), _T("DiagnosticMode"), dwVal);
		return dwVal == 1;
	}

	void DebugMsg(const TCHAR* szFormat, ...)
	{	
		if(IsInDiagnosticMode() == 1
#if defined(_DEBUG)
			|| 1
#endif
			)
		{
			//Initialise the variable arguments list
			va_list arglist; 
			va_start(arglist, szFormat);

			TCHAR szFormat2[4096];
			_sntprintf_s(szFormat2, sizeof(szFormat2), _T("WinA1314|%s"), szFormat);

			//format original parameters
			TCHAR szBuffer[4096];
			_vsntprintf_s(szBuffer, sizeof(szBuffer), szFormat2, arglist); 

			_tprintf(szBuffer);
			OutputDebugString(szBuffer);

			va_end(arglist);
		}

	}

	tstring GetLastErrorStr(DWORD dwErrorCode)
	{
		LPTSTR lpMsgBuf = NULL;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dwErrorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);

		return tstring(lpMsgBuf);
	}

	bool GetRegKeyDwordValue(const HKEY& hKey, const tstring& location, const tstring& name, DWORD& value)
	{
		HKEY key;
		DWORD bufLen = sizeof(DWORD);

		long ret = RegOpenKeyEx(hKey, location.c_str(), 0, KEY_QUERY_VALUE, &key);
		if(ret != ERROR_SUCCESS)
			return false;

		ret = RegQueryValueEx(key, name.c_str(), 0, 0, (LPBYTE)&value, &bufLen);
		RegCloseKey(key);
		if((ret != ERROR_SUCCESS) || (bufLen > 1024*sizeof(TCHAR)))
			return false;

		return true;
	}	

	void Tokenize(const tstring& str, vector<tstring>& tokens, const tstring& delimiters)
	{
		string::size_type lastPos = str.find_first_not_of(delimiters, 0);
		string::size_type pos     = str.find_first_of(delimiters, lastPos);

		while (tstring::npos != pos || tstring::npos != lastPos)
		{
			tokens.push_back(str.substr(lastPos, pos - lastPos));
			lastPos = str.find_first_not_of(delimiters, pos);
			pos = str.find_first_of(delimiters, lastPos);
		}
	}

}