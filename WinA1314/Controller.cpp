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
#include "Controller.h"
#include "AppleKeyboardHID.h"
#include "Interceptor.h"
#include "Globals.h"

const CInterceptor::InterceptionKeyStroke delUp  = {0x53, CInterceptor::INTERCEPTION_KEY_UP | CInterceptor::INTERCEPTION_KEY_E0};
const CInterceptor::InterceptionKeyStroke delDown  = {0x53, CInterceptor::INTERCEPTION_KEY_DOWN | CInterceptor::INTERCEPTION_KEY_E0};

const CInterceptor::InterceptionKeyStroke ctrlUp = {0x1d, CInterceptor::INTERCEPTION_KEY_UP};
const CInterceptor::InterceptionKeyStroke ctrlDown = {0x1d, CInterceptor::INTERCEPTION_KEY_DOWN};

//This is a dummy key code to internally represent the Fn key
const CInterceptor::InterceptionKeyStroke fnDown  = {0xff, CInterceptor::INTERCEPTION_KEY_DOWN};
const CInterceptor::InterceptionKeyStroke fnUp = {0xff, CInterceptor::INTERCEPTION_KEY_UP};

CController::CController(void)
{
	m_bVirtualFnPressed = false;
	m_bFnPressed = false;
	m_bEjectPressed = false;

	ZeroMemory(m_WaitHandles, sizeof(m_WaitHandles));

	m_hShutdownEvent = CreateEvent(0, TRUE, FALSE, 0);	

	LoadRegKeyMappings();
}

CController::~CController(void)
{
	Cleanup();
}

bool CController::LoadRegKeyMappings() 
{ 
	static const int MAX_KEY_LENGTH = 255;
	static const int MAX_VALUE_NAME = 16383;

	HKEY hKey;
	LONG dwRegOPenKey = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\WinA1314\\Map"), 0, KEY_READ, &hKey);
	if(dwRegOPenKey != ERROR_SUCCESS)
	{
		tstring strLastError = Globals::GetLastErrorStr(dwRegOPenKey);
		Globals::DebugMsg(_T("LoadRegKeyMappings(): RegOpenKeyEx failed: %s.\n"), strLastError.c_str());

		return false;
	} 		

	TCHAR achKey[255];        
	TCHAR achClass[MAX_PATH] = _T("");
	DWORD cchClassName = MAX_PATH; 
	DWORD cSubKeys = 0;              
	DWORD cbName, cbMaxSubKey, cchMaxClass, cValues, cchMaxValue, cbMaxValueData, cbSecurityDescriptor;
	FILETIME ftLastWriteTime;  

	TCHAR achValue[MAX_VALUE_NAME]; 
	DWORD cchValue = MAX_VALUE_NAME; 

	//Get the class name and the value count. 
	DWORD ret = RegQueryInfoKey(hKey, achClass, &cchClassName, NULL, &cSubKeys, &cbMaxSubKey, &cchMaxClass, 
		&cValues, &cchMaxValue, &cbMaxValueData, &cbSecurityDescriptor, &ftLastWriteTime);    
	if(ret != ERROR_SUCCESS)
	{
		tstring strLastError = Globals::GetLastErrorStr(ret);
		Globals::DebugMsg(_T("LoadRegKeyMappings(): RegQueryInfoKey failed: %s.\n"), strLastError.c_str());

		RegCloseKey(hKey);
		return false;
	}

	if (cSubKeys)
	{
		for (DWORD i = 0; i < cSubKeys; i++) 
		{ 
			cbName = MAX_KEY_LENGTH;
			ret = RegEnumKeyEx(hKey, i, achKey,  &cbName, NULL, NULL, NULL, &ftLastWriteTime); 
			if (ret == ERROR_SUCCESS) 
			{
				Globals::DebugMsg(_T("LoadRegKeyMappings(): (%d) %s\n"), i+1, achKey);
			}
		}
	} 

	BYTE* buffer = new BYTE[cbMaxValueData];
	ZeroMemory(buffer, cbMaxValueData);
	if (cValues) 
	{
		for (DWORD i = 0, ret = ERROR_SUCCESS; i < cValues; i++) 
		{ 
			cchValue = MAX_VALUE_NAME; 
			achValue[0] = 0; 
			ret = RegEnumValue(hKey, i, achValue, &cchValue, NULL, NULL, NULL, NULL);
			if (ret == ERROR_SUCCESS ) 
			{ 
				DWORD lpData = cbMaxValueData;
				ZeroMemory(buffer, cbMaxValueData);

				LONG dwRes = RegQueryValueEx(hKey, achValue, 0, NULL, buffer, &lpData);
				Globals::DebugMsg(_T("LoadRegKeyMappings(): %s : %s\n"), achValue, buffer); 

				vector<tstring> tokens;
				Globals::Tokenize((TCHAR*)buffer, tokens, _T(",: "));

				if(tokens.size() >= 9)
				{
					CInterceptor::InterceptionKeyStroke srcKey;					
					ZeroMemory(&srcKey, sizeof(CInterceptor::InterceptionKeyStroke));

					_stscanf_s(tokens[0].c_str(), _T("%i"), &srcKey.code);
					_stscanf_s(tokens[1].c_str(), _T("%i"), &srcKey.state);
					_stscanf_s(tokens[2].c_str(), _T("%i"), &srcKey.information);

					if(m_KeyMappings.find(srcKey) == m_KeyMappings.end())
					{
						CController::KeyMapping dstKeyMapping;
						ZeroMemory(&dstKeyMapping, sizeof(CController::KeyMapping));

						_stscanf_s(tokens[3].c_str(), _T("%i"), &dstKeyMapping.normal.code);
						_stscanf_s(tokens[4].c_str(), _T("%i"), &dstKeyMapping.normal.state);
						_stscanf_s(tokens[5].c_str(), _T("%i"), &dstKeyMapping.normal.information);

						_stscanf_s(tokens[6].c_str(), _T("%i"), &dstKeyMapping.fn.code);
						_stscanf_s(tokens[7].c_str(), _T("%i"), &dstKeyMapping.fn.state);
						_stscanf_s(tokens[8].c_str(), _T("%i"), &dstKeyMapping.fn.information);

						//Initialise destination initial key states to UP so that the ProcessKey() routine initialises correctly
						dstKeyMapping.normal.state |= 0x1;
						dstKeyMapping.fn.state |= 0x1;

						m_KeyMappings[srcKey] = dstKeyMapping;

						Globals::DebugMsg(_T("LoadRegKeyMappings(): (%x,%x,%x) -> Normal: (%x,%x,%x) WithFn: (%x,%x,%x).\n"), srcKey.code, srcKey.state, srcKey.information, 
							dstKeyMapping.normal.code, dstKeyMapping.normal.state, dstKeyMapping.normal.information,
							dstKeyMapping.fn.code, dstKeyMapping.fn.state, dstKeyMapping.fn.information);
					}
				}
			} 
		}
	}

	RegCloseKey(hKey);
	delete[] buffer;

	return true;
}

void CController::Stop()
{
	Globals::DebugMsg(_T("CController::Stop()\n"));

	if(m_hShutdownEvent)
		SetEvent(m_hShutdownEvent);
}

void CController::Cleanup()
{
	if(m_bFnPressed && m_KbF.IsOpen())
	{		
		m_KbF.Write(ctrlUp);
	}

	if(!m_hShutdownEvent)
	{
		CloseHandle(m_hShutdownEvent);
		m_hShutdownEvent = NULL;
	}

	m_KbF.Close();
	CAppleKeyboardHID::Instance()->Close();
}

CController::EventsLoopStatusEnum CController::EventLoop()
{
	int nWaitHandles = TryConnect() ? 3 : 1;
	while(1)
	{
		m_WaitHandles[0] = m_hShutdownEvent;
		m_WaitHandles[1] = m_KbF.m_hEvent;
		m_WaitHandles[2] = CAppleKeyboardHID::Instance()->m_hEvent;

		DWORD dwWaitObject = WaitForMultipleObjectsEx(nWaitHandles, m_WaitHandles, FALSE, 5000, TRUE);
		CInterceptor::InterceptionKeyStroke key;
		switch(dwWaitObject)
		{
		case WAIT_OBJECT_0 + 0:
			Globals::DebugMsg(_T("CController::EventLoop(): Shutdown event.\n"));
			ResetEvent(m_hShutdownEvent);
			return StopRequested;
		case WAIT_OBJECT_0 + 1:
			if(m_KbF.Read(&key) > 0)
				ProcessKey(key);
			break;
		case WAIT_OBJECT_0 + 2:
			ProcessSpecialKey(CAppleKeyboardHID::Instance()->GetKey());
			CAppleKeyboardHID::Instance()->ReadNextBlock();
			break;
		case WAIT_IO_COMPLETION:
			break;
		case WAIT_TIMEOUT:
			//Globals::DebugMsg(_T("CController::EventLoop(): WAIT_TIMEOUT.\n"));
			if(!CAppleKeyboardHID::Instance()->IsOpen() || !m_KbF.IsOpen())
				nWaitHandles = TryConnect() ? 3 : 1;
			break;
		case WAIT_FAILED:
			Globals::DebugMsg(_T("CController::EventLoop(): WAIT_FAILED.\n"));
			SleepEx(5000, true);
			nWaitHandles = 1;
			break;
		default:
			Globals::DebugMsg(_T("CController::EventLoop(): (default).\n"));
			nWaitHandles = 1;
			break;
		}
	}
}

bool CController::TryConnect()
{
	if(CAppleKeyboardHID::Instance()->Open() && m_KbF.Open())
	{
		Globals::DebugMsg(_T("CController::TryConnect(): Connected.\n"));
		return true;
	}

	m_KbF.Close();
	CAppleKeyboardHID::Instance()->Close();
	return false;
}


void CController::ProcessKey(const CInterceptor::InterceptionKeyStroke& srcKey)
{
	Globals::DebugMsg(_T("CController::ProcessKey(): 0x%x 0x%x 0x%x\n"), srcKey.code, srcKey.state, srcKey.information);

	map<CInterceptor::InterceptionKeyStroke, KeyMapping, InterceptionKeyStrokeComparer>::iterator pos = m_KeyMappings.find(srcKey);
	if(pos != m_KeyMappings.end())
	{										
		//If Fn key is DOWN and mapping for destination fn key exists
		CInterceptor::InterceptionKeyStroke* pDstFnKey = &pos->second.fn;
		if(m_bVirtualFnPressed && pDstFnKey && pDstFnKey->code != 0)
		{
			//Current dst fn key state
			const bool bDstFnKeyPressed = (pDstFnKey->state & 0x1) == CInterceptor::INTERCEPTION_KEY_DOWN;

			bool bSrcKeyDown = ((srcKey.state & 0x1) == CInterceptor::INTERCEPTION_KEY_DOWN);
			if(bSrcKeyDown) //Source key went DOWN
			{
				//bool bAllowAutoRepeat = (pos->second.flags & 0x1) == 1;		
				//if(!bDstFnKeyPressed || bAllowAutoRepeat)
				//{						
					Globals::DebugMsg(_T("CController::ProcessKey(): * Mapped Fn key down *.\n"));

					//Set destination key state to down
					pDstFnKey->state ^= CInterceptor::INTERCEPTION_KEY_UP;
					Globals::DebugMsg(_T("CController::ProcessKey(): Write 0x%x,0x%x,0x%x.\n"), pDstFnKey->code, pDstFnKey->state, pDstFnKey->information);

					m_KbF.Write(*pDstFnKey);
				//}

				return;
			}
			else if(bDstFnKeyPressed) //Source key went UP AND destination key currently DOWN
			{					
				Globals::DebugMsg(_T("CController::ProcessKey(): * Mapped Fn key up *.\n"));

				//Set destination key state to up
				pDstFnKey->state |= CInterceptor::INTERCEPTION_KEY_UP;
				Globals::DebugMsg(_T("CController::ProcessKey(): Write 0x%x,0x%x,0x%x.\n"), pDstFnKey->code, pDstFnKey->state, pDstFnKey->information);

				m_KbF.Write(*pDstFnKey);

				return;
			}
		}

		//If mapping for normal destination key exists
		CInterceptor::InterceptionKeyStroke* pDstKey = &pos->second.normal;
		if(pDstKey && pDstKey->code != 0)
		{
			Globals::DebugMsg(_T("CController::ProcessKey(): * Mapped normal key %s *.\n"), (srcKey.state & 0x1) == 1 ? _T("UP") : _T("DOWN"));

			pDstKey->state = (srcKey.state & 0x1) | (pDstKey->state & 0xfffe);				
			Globals::DebugMsg(_T("CController::ProcessKey(): Write 0x%x,0x%x,0x%x.\n"), pDstKey->code, pDstKey->state, pDstKey->information);

			if(pDstKey->code == fnDown.code)
				m_bVirtualFnPressed = (pDstKey->state & CInterceptor::INTERCEPTION_FILTER_KEY_DOWN) == CInterceptor::INTERCEPTION_KEY_DOWN;
			else
				m_KbF.Write(*pDstKey);

			return;
		}
	}			

	if(srcKey.code == fnDown.code)
		m_bVirtualFnPressed = (srcKey.state & CInterceptor::INTERCEPTION_FILTER_KEY_DOWN) == CInterceptor::INTERCEPTION_KEY_DOWN;
	else
		m_KbF.Write(srcKey);	
}

void CController::ProcessSpecialKey(const CAppleKeyboardHID::SpecialKey& skey)
{
	Globals::DebugMsg(_T("CController::ProcessSpecialKey(): 0x%x 0x%x\n"), skey.b0, skey.b1);

	if(skey.b0 == 0x11)
	{
		if((skey.b1 == 0x8 || skey.b1 == 0x18) && !m_bEjectPressed) //Eject || FnEject
		{
			Globals::DebugMsg(_T("CController::ProcessSpecialKey(): Eject key down\n"));
			m_bEjectPressed = true;
			
			m_KbF.Write(CInterceptor::InterceptionKeyStroke(delDown));
		}
		
		if((skey.b1== 0x10 || skey.b1 == 0x18) && !m_bFnPressed) //Fn || FnEject
		{
			Globals::DebugMsg(_T("CController::ProcessSpecialKey(): Physical Fn key down\n"));
			m_bFnPressed = true;

			ProcessKey(fnDown);
		}

		if(skey.b1 == 0) //None pressed
		{
			if(m_bFnPressed)
			{
				Globals::DebugMsg(_T("CController::ProcessSpecialKey(): Physical Fn key up\n"));
				m_bFnPressed = false;	

				ProcessKey(fnUp);
			}

			if(m_bEjectPressed)
			{
				Globals::DebugMsg(_T("CController::ProcessSpecialKey(): Eject key up\n"));
				m_bEjectPressed = false;
				
				m_KbF.Write(CInterceptor::InterceptionKeyStroke(delUp));
			}
		}
	}		
	else if(skey.b0 == 0x13) //Power button events
	{
		switch (skey.b1)
		{
		case 0x3:
			Globals::DebugMsg(_T("CController::ProcessSpecialKey(): Power button down\n"));
			break;
		case 0x2:
			Globals::DebugMsg(_T("CController::ProcessSpecialKey(): Power button up\n"));
			break;
		case 0x1:
			Globals::DebugMsg(_T("CController::ProcessSpecialKey(): Power button longpress\n"));
			break;
		default:
			break;
		}
	}
				
}