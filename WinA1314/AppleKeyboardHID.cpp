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
#include "AppleKeyboardHID.h"

extern "C" 
{
#include <hidsdi.h> 
} //extern "C"

#include <setupapi.h>
#include <initguid.h>
#include "Globals.h"

CAppleKeyboardHID* CAppleKeyboardHID::s_Instance = NULL;

CAppleKeyboardHID* CAppleKeyboardHID::Instance()
{
	if(s_Instance == NULL)
		s_Instance = new CAppleKeyboardHID();
	return s_Instance;
}

CAppleKeyboardHID::CAppleKeyboardHID(void)
{
	m_hHid = NULL;
	m_pIAsyncNotifier = NULL;
	m_hEvent = NULL;
}

CAppleKeyboardHID::~CAppleKeyboardHID(void)
{
	Close();
}

void CAppleKeyboardHID::Close()
{
	if(m_hHid)
	{
		if(CancelIo(m_hHid) == FALSE)
			Globals::DebugMsg(_T("CancelIo() failed: %d\n"), GetLastError());

		CloseHandle(m_hHid);
		m_hHid = NULL;
	}
		
	if(m_hEvent)
	{
		SetEvent(m_hEvent);
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}	
}

bool CAppleKeyboardHID::IsOpen()
{
	return m_hHid != NULL;
}

bool CAppleKeyboardHID::Open(IAsyncNotifier* pIAsyncNotifier)
{
	if(IsOpen())
		return true;

	if(!OpenInternal()) //Sets m_hHid
	{
		Globals::DebugMsg(_T("CAppleKeyboardHID::Open(): Failed to open HID device\n"));
		return false;
	}

	m_hEvent = CreateEvent(0, TRUE, FALSE, 0);
	m_pIAsyncNotifier = pIAsyncNotifier;
	if(!ReadNextBlock())
	{
		Close();

		return false;
	}

	return true;
}

bool CAppleKeyboardHID::OpenInternal()
{	
	if(IsOpen())
	{
		Globals::DebugMsg(_T("CAppleKeyboardHID::HidOpen(): Failed, already open.\n"));
		return false;
	}

	//
	//API function: HidD_GetHidGuid
	//Get the GUID for all system HIDs.
	//Returns: the GUID in HidGuid.
	//
	GUID hidGuid;
	::HidD_GetHidGuid(&hidGuid);	

	//
	//API function: SetupDiGetClassDevs
	//Returns: a handle to a device information set for all installed devices.
	//Requires: the GUID returned by GetHidGuid.
	//
	HDEVINFO hDevInfo= ::SetupDiGetClassDevs(&hidGuid, // ClassGuid
						NULL, // Enumerator
						NULL, // hwndParent
						DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);	// Flags

	DWORD memberIndex = 0;
	while(m_hHid == NULL)
	{
		SP_DEVICE_INTERFACE_DATA devInfoData;
		devInfoData.cbSize = sizeof(devInfoData);
		BOOL bRes = ::SetupDiEnumDeviceInterfaces(hDevInfo, // DeviceInfoSet
							0, // DeviceInfoData
							&hidGuid, // InterfaceClassGuid
							memberIndex, // MemberIndex
							&devInfoData); // DeviceInterfaceData
		if(!bRes)
			break; //no more devices to check.
						
		//Get the Length value.
		//The call will return with a "buffer too small" error which can be ignored.
		DWORD Length;
		bRes = ::SetupDiGetDeviceInterfaceDetail(hDevInfo, // DeviceInfoSet
			&devInfoData, // DeviceInterfaceData
			NULL, // DeviceInterfaceDetailData
			0, // DeviceInterfaceDetailDataSize
			&Length, // RequiredSize
			NULL); // DeviceInfoData

		PSP_DEVICE_INTERFACE_DETAIL_DATA detailData= (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(Length);
		detailData -> cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		//Call the function again, this time passing it the returned buffer size.
		DWORD dwRequired;
		bRes = ::SetupDiGetDeviceInterfaceDetail(hDevInfo, // DeviceInfoSet
			&devInfoData, // DeviceInterfaceData
			detailData,	 // DeviceInterfaceDetailData
			Length, // DeviceInterfaceDetailDataSize
			&dwRequired, // RequiredSize
			NULL); // DeviceInfoData

		//
		//API function: CreateFile
		//Returns: a handle that enables reading and writing to the device.
		//Requires:
		//The DevicePath in the detailData structure
		//returned by SetupDiGetDeviceInterfaceDetail.
		//
		m_hHid = ::CreateFile(detailData->DevicePath, // lpFileName
			GENERIC_READ|GENERIC_WRITE, // dwDesiredAccess
			FILE_SHARE_READ|FILE_SHARE_WRITE, // dwShareMode
			NULL, // lpSecurityAttributes
			OPEN_EXISTING, // dwCreationDisposition
			FILE_FLAG_OVERLAPPED/*0*/, // dwFlagsAndAttributes
			NULL); // hTemplateFile

		Globals::DebugMsg(_T("CAppleKeyboardHID::HidOpen(): %s\n"), detailData->DevicePath);

		//
		//API function: HidD_GetAttributes
		//Requests information from the device.
		//Requires: the handle returned by CreateFile.
		//Returns: a HIDD_ATTRIBUTES structure containing
		//the Vendor ID, Product ID, and Product Version Number.
		//Use this information to decide if the detected device is
		//the one we're looking for.
		//
		HIDD_ATTRIBUTES att;
		att.Size= sizeof(att);
		BOOLEAN result4= ::HidD_GetAttributes(m_hHid, &att);

		tstring strDevPathLc = detailData->DevicePath;
		transform(strDevPathLc.begin(), strDevPathLc.end(), strDevPathLc.begin(), ::tolower);

		
		if (att.VendorID  == Globals::VID_APPLE 
			&& (att.ProductID == Globals::PID_APPLE_ALU_WIRELESS_ANSI ||
			att.ProductID == Globals::PID_APPLE_ALU_WIRELESS_ISO ||
			att.ProductID == Globals::PID_APPLE_ALU_WIRELESS_JIS ||
			att.ProductID == Globals::PID_APPLE_ALU_WIRELESS_2009_ANSI ||
			att.ProductID == Globals::PID_APPLE_ALU_WIRELESS_2009_ISO ||
			att.ProductID == Globals::PID_APPLE_ALU_WIRELESS_2009_JIS ||
			att.ProductID == Globals::PID_APPLE_ALU_WIRELESS_2011_ANSI ||
			att.ProductID == Globals::PID_APPLE_ALU_WIRELESS_2011_ISO)
			&& strDevPathLc.find(_T("col03")) != tstring::npos //This TLC interface is for Fn, Eject etc.
			)
		{
			Globals::DebugMsg(_T("CAppleKeyboardHID::HidOpen(): TLC 03 interface matched: %s\n"), detailData->DevicePath);		
		}
		else
		{				
			CloseHandle(m_hHid);
			m_hHid = NULL;
		}

		free(detailData);

		memberIndex++;
	}

	//Free the memory reserved for hDevInfo by SetupDiClassDevs.
	::SetupDiDestroyDeviceInfoList(hDevInfo);

	return m_hHid != NULL;
}


void WINAPI CAppleKeyboardHID::BlockComplete(DWORD dwErrorCode, DWORD dwNumberOfBytesTransferred, LPOVERLAPPED lpOverlapped)
{
	LPOVERLAPPEDEX pOverlappedEx = static_cast<LPOVERLAPPEDEX>(lpOverlapped);
	CAppleKeyboardHID* this_ = reinterpret_cast<CAppleKeyboardHID*>(pOverlappedEx->pCustomData);

	SetEvent(this_->m_hEvent);

	if(this_->m_pIAsyncNotifier != NULL)
		this_->m_pIAsyncNotifier->OnReadComplete();
}

bool CAppleKeyboardHID::ReadNextBlock()
{
	ZeroMemory(&ov, sizeof(ov));
	ov.pCustomData = this;

	ResetEvent(m_hEvent);

	BOOL bResult = ReadFileEx(m_hHid, &m_Key, sizeof(SpecialKey), &ov, BlockComplete);
	if(!bResult) 
	{
		Globals::DebugMsg(_T("CAppleKeyboardHID::ReadNextBlock(): ReadFileEx() failed.\n"));
		Close();
	}

	return bResult == 1;
}

