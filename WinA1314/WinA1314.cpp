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
#include "Controller.h"

#define SERVICE_NAME _T("WinA1314")

CController g_Controller;
SERVICE_STATUS g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;

VOID WINAPI ServiceMain (DWORD argc, LPTSTR *argv);
BOOL WINAPI ConsoleHandler(DWORD dwCtrlType);
DWORD WINAPI ServiceCtrlHandlerEx(DWORD dwControlCode, DWORD dwEventType, LPVOID x, LPVOID y);

int _tmain(int argc, _TCHAR* argv[])
{
	Globals::DebugMsg(_T("_tmain()\n"));	

	//Run as console application
	if(argc == 2 && _tcscmp(argv[1], _T("-a")) == 0)
	{
		if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE) == FALSE)
		{
			Globals::DebugMsg(_T("_tmain(): Failed to install console handler.\n"));
			return -1;
		}

		g_Controller.EventLoop(); //Blocking call
	}
	else
	{
		//Run as service
		SERVICE_TABLE_ENTRY ServiceTable[] = 
		{
			{SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
			{NULL, NULL}
		};

		if (StartServiceCtrlDispatcher (ServiceTable) == FALSE)
		{
			Globals::DebugMsg(_T("_tmain(): StartServiceCtrlDispatcher returned error\n"));
			return GetLastError ();
		}
	}

    return 0;
}

BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
	Globals::DebugMsg(_T("ConsoleHandler()\n"));

    switch(CEvent)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
		g_Controller.Stop();
	default:
		break;
    }

    return TRUE;
}

VOID WINAPI ServiceMain (DWORD argc, LPTSTR *argv)
{
	Globals::DebugMsg(_T("ServiceMain()\n"));

    DWORD Status = E_FAIL;

    g_StatusHandle = RegisterServiceCtrlHandlerEx(SERVICE_NAME, ServiceCtrlHandlerEx, 0);
    if (g_StatusHandle == NULL) 
    {
        Globals::DebugMsg(_T("ServiceMain(): RegisterServiceCtrlHandler returned error\n"));
        return;
    }

    // Tell the service controller we are starting
    ZeroMemory (&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE) 
    {
        Globals::DebugMsg(_T("ServiceMain(): SetServiceStatus returned error\n"));
    }

    //Perform tasks neccesary to start the service here
    Globals::DebugMsg(_T("ServiceMain(): Performing Service Start Operations\n"));
   
    //Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
	    Globals::DebugMsg(_T("ServiceMain(): SetServiceStatus returned error\n"));
    }

	static const GUID GUID_DEVINTERFACE_HID = { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };
	DEV_BROADCAST_DEVICEINTERFACE m_NotificationFilter;
	HDEVNOTIFY m_hDeviceNotify;
	ZeroMemory( &m_NotificationFilter, sizeof(m_NotificationFilter));
	m_NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	m_NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	m_NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_HID;

	m_hDeviceNotify = RegisterDeviceNotification((HANDLE)g_StatusHandle, 
		&m_NotificationFilter, DEVICE_NOTIFY_SERVICE_HANDLE);

	if(m_hDeviceNotify == NULL)
	{
		tstring strLastError = Globals::GetLastErrorStr(GetLastError());
		Globals::DebugMsg(_T("ServiceMain(): RegisterDeviceNotification failed: %s.\n"), strLastError.c_str());
	}
	else
		Globals::DebugMsg(_T("ServiceMain(): RegisterDeviceNotification success.\n"));
	
	//Change thread priority
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	///
	g_Controller.EventLoop(); //Blocking call
	///

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
	    Globals::DebugMsg(_T("ServiceMain(): SetServiceStatus returned error\n"));
    }
    
	Globals::DebugMsg(_T("ServiceMain(): *** EXIT ***\n"));

    return;
}

DWORD WINAPI ServiceCtrlHandlerEx(DWORD dwControlCode, DWORD dwEventType, LPVOID x, LPVOID y)
{
    Globals::DebugMsg(_T("ServiceCtrlHandlerEx()\n"));

    switch (dwControlCode) 
	{
	case SERVICE_CONTROL_STOP :
		Globals::DebugMsg(_T("ServiceCtrlHandlerEx(): SERVICE_CONTROL_STOP Request\n"));

		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		//Perform tasks neccesary to stop the service here 
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;

		if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			Globals::DebugMsg(_T("ServiceCtrlHandlerEx(): SetServiceStatus returned error\n"));
		}

		g_Controller.Stop(); 

		break;
	case SERVICE_CONTROL_DEVICEEVENT:
		Globals::DebugMsg(_T("ServiceCtrlHandlerEx(): SERVICE_CONTROL_DEVICEEVENT\n"));
		switch(dwEventType)
		{
		case DBT_DEVICEARRIVAL:
			Globals::DebugMsg(_T("ServiceCtrlHandlerEx(): DBT_DEVICEARRIVAL\n"));
			//PDEV_BROADCAST_DEVICEINTERFACE pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)x;
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			Globals::DebugMsg(_T("ServiceCtrlHandlerEx(): DBT_DEVICEREMOVECOMPLETE\n"));
			break;
		}
     default:
         break;
    }

	return 0;
}

