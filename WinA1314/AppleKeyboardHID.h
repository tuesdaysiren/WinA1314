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

#include <dbt.h>

class CAppleKeyboardHID
{
public:
	typedef struct OVERLAPPEDEX : public OVERLAPPED
	{
		LPVOID pCustomData;
	} *LPOVERLAPPEDEX;

	class IAsyncNotifier
	{
	public:
		virtual void OnReadComplete()=0;
	};

	typedef struct
	{
		unsigned char b0, b1;
	} SpecialKey;

public:
	HANDLE m_hEvent;

public:	
	static CAppleKeyboardHID* Instance(); //Singleton instance
	virtual ~CAppleKeyboardHID(void);

	bool IsOpen();
	void Close();
	bool Open(IAsyncNotifier* pIAsyncNotifier = NULL);
	bool ReadNextBlock();
	const SpecialKey& GetKey() { return m_Key; }

protected:
	CAppleKeyboardHID(void); //Disallow public constructor
	bool OpenInternal();
	static void WINAPI BlockComplete(DWORD dwErrorCode, DWORD dwNumberOfBytesTransferred, LPOVERLAPPED lpOverlapped);
	
private:
	static CAppleKeyboardHID* s_Instance; //Singleton instance
	IAsyncNotifier* m_pIAsyncNotifier;
	SpecialKey m_Key;
	HANDLE m_hHid;
	OVERLAPPEDEX ov;
};

