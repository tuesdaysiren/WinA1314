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

#include "AppleKeyboardHID.h"
#include "Interceptor.h"

class CController
{
public:
	typedef enum EventsLoopStatusEnum {Ok, StopRequested, FatalError};

	typedef struct tagKeyMapping 
	{ 
		CInterceptor::InterceptionKeyStroke normal; 
		CInterceptor::InterceptionKeyStroke fn; 
	} KeyMapping;

	typedef struct tagInterceptionKeyStrokeComparer
	{
		bool operator()(const CInterceptor::InterceptionKeyStroke& lhs, const CInterceptor::InterceptionKeyStroke& rhs) const
		{
			if(lhs.code < rhs.code)
				return true;
			if(lhs.code > rhs.code)
				return false;
			if((lhs.state & ~CInterceptor::INTERCEPTION_KEY_UP) < (rhs.state & ~CInterceptor::INTERCEPTION_KEY_UP))
				return true;
			if((lhs.state & ~CInterceptor::INTERCEPTION_KEY_UP) > (rhs.state & ~CInterceptor::INTERCEPTION_KEY_UP))
				return false;

			return false;
		}
	} InterceptionKeyStrokeComparer;	

public:
	CController(void);
	~CController(void);

	EventsLoopStatusEnum EventLoop();
	void Stop();

protected:
	bool TryConnect();
	void ProcessKey(const CInterceptor::InterceptionKeyStroke& key);
	void ProcessSpecialKey(const CAppleKeyboardHID::SpecialKey& skey);
	void Cleanup();
	bool LoadRegKeyMappings();

protected:
	HANDLE m_hShutdownEvent;
	CInterceptor m_KbF;
	HANDLE m_WaitHandles[3];

	map<CInterceptor::InterceptionKeyStroke, KeyMapping, InterceptionKeyStrokeComparer> m_KeyMappings;

	bool m_bVirtualFnPressed;
	bool m_bEjectPressed;
	bool m_bFnPressed;	
};



