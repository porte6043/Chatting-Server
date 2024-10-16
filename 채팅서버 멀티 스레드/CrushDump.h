#ifndef __LIB_CRUSH_DUMP__
#define __LIB_CRUSH_DUMP__

#include <iostream>
#include <Windows.h>
#include <cstdlib>		// For _invalid_parameter_handler
#include <crtdbg.h>		// For _CrtSetReportMode
#include <DbgHelp.h>	// For _MINIDUMP_EXCEPTION_INFORMATION, MiniDumpWriteDump 
#pragma comment(lib, "Dbghelp.lib")

// Windows.h 위에 있을 경우 에러 발생
#include <Psapi.h>		// For 	PROCESS_MEMORY_COUNTERS, GetProcessMemoryInfo
#undef PSAPI_VERSION
#define PSAPI_VERSION 1
#pragma comment(lib, "psapi.lib")

class CCrushDump
{
public:
	CCrushDump()
	{
		//_DumpCount = 0;

		_invalid_parameter_handler OldHandler, NewHandler;

		NewHandler = myInvalidParameterHandler;
		OldHandler = _set_invalid_parameter_handler(NewHandler);	// crt 함수에 null 포인터 등을 넣었을 때..
		_CrtSetReportMode(_CRT_WARN, 0);		// CRT 오류 메시지 표시 중단, 바로 덤프로 남도록.
		_CrtSetReportMode(_CRT_ASSERT, 0);		// CRT 오류 메시지 표시 중단, 바로 덤프로 남도록.
		_CrtSetReportMode(_CRT_ERROR, 0);		// CRT 오류 메시지 표시 중단, 바로 덤프로 남도록.

		_CrtSetReportHook(_custom_Report_hook);

		//----------------------------------------------------------------------------
		// pure virtual function called 에러 핸들러를 사용자 정의 함수로 우회시킨다.
		//----------------------------------------------------------------------------
		_set_purecall_handler(myPurecallHandler);

		SetHandlerDump();
	}

	static void Crush()
	{
		int* p = nullptr;
		*p = 0;
	}

	static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
	{
		static long _DumpCount = 0;

		int iWorkingMemory = 0;
		SYSTEMTIME stNowTime;

		//----------------------------------------------------------------------------
		// 여러 스레드에서 동시에 덤프가 남을 때 동일한 이름의 파일을 만들면 이미 Write 상태의
		// 파일이 있기 때문에 예외처리가 끝난다. 그래서 첫 번째 덤프가 남기 전에 프로세스가 종료된다.
		// 그렇기 때문에 InterlockedIncrement를 이용하여 파일이름을 다르게 한다.
		//----------------------------------------------------------------------------
		long DumpCount = InterlockedIncrement(&_DumpCount);	

		//----------------------------------------------------------------------------
		// 현재 프로세스의 메모리 사용량을 얻어온다
		//----------------------------------------------------------------------------
		HANDLE hProcess = 0;
		PROCESS_MEMORY_COUNTERS pmc;
		
		hProcess = GetCurrentProcess();
		if (hProcess == NULL)
			return 0;

		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			iWorkingMemory = (int)(pmc.WorkingSetSize / 1024 / 1024);
		}
		CloseHandle(hProcess);

		//----------------------------------------------------------------------------
		// 현재 날짜와 시간을 알아온다.
		//----------------------------------------------------------------------------
		WCHAR FileName[MAX_PATH];
		GetLocalTime(&stNowTime);
		wsprintf(FileName, L"Dump_%d%02d%02d_%02d.%02d.%02d_%d_%dMB.dmp",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, DumpCount, iWorkingMemory);

		wprintf(L"\n\n\n!!! Crash Error!!! %d.%d.%d / %d:%d:%d \n",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond);
		//wprintf(L"Now Save Dump File...\n");

		HANDLE hDumpFile = ::CreateFile(FileName,
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hDumpFile != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION MinidumpExceptionInformation;
			MinidumpExceptionInformation.ThreadId = ::GetCurrentThreadId();
			MinidumpExceptionInformation.ExceptionPointers = pExceptionPointer;
			MinidumpExceptionInformation.ClientPointers = TRUE;

			MiniDumpWriteDump(GetCurrentProcess(),
				GetCurrentProcessId(),
				hDumpFile,
				MiniDumpWithFullMemory,
				&MinidumpExceptionInformation,
				NULL,
				NULL);
			CloseHandle(hDumpFile);
			  
			wprintf(L"CrashDump Save Finish!!!");
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void SetHandlerDump()
	{
		SetUnhandledExceptionFilter(MyExceptionFilter);
	}

	// Invaild Parameter handler
	static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
	{
		Crush();
	}

	static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue)
	{
		Crush();
		return true;
	}

	static void myPurecallHandler(void)
	{
		Crush();
	}

	//static long _DumpCount;
};

//long CCrushDump::_DumpCount = 0;
#endif
