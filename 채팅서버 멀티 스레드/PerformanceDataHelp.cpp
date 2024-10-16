#include "PerformanceDataHelp.h"


CPerformanceDataHelp::CPerformanceDataHelp() : 
	ProcessCpuTotalQuery{}, ProcessCpuKernelQuery{},
	ProcessCpuUserQuery{}, ProcessNonPagePoolQuery{},
	ProcessUsingMemoryQuery{}
{
	WCHAR buf[256] = { 0, };
	DWORD bufLen = sizeof(buf);
	DWORD pid = GetCurrentProcessId();
	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);	// 프로세스 pid로 프로세스 핸들 검색
	QueryFullProcessImageName(hProc, 0, buf, &bufLen);							// 프로세스 핸들을 통해 전체 경로 찾기
	CloseHandle(hProc);

	// 파일 경로에서 디렉토리 구분자를 찾음
	std::wstring ProcessPath = buf;
	size_t StartIndex = ProcessPath.find_last_of(L"\\/");
	size_t LastIndex = ProcessPath.find_last_of(L".");

	// 프로세스 이름
	std::wstring ProcessName = ProcessPath.substr(StartIndex + 1, LastIndex - StartIndex - 1);

	// 쿼리문 입력
	StringCchPrintfW(ProcessCpuTotalQuery, 256, L"\\Process(%s)\\%% Processor Time", ProcessName.c_str());
	StringCchPrintfW(ProcessCpuKernelQuery, 256, L"\\Process(%s)\\%% Privileged Time", ProcessName.c_str());
	StringCchPrintfW(ProcessCpuUserQuery, 256, L"\\Process(%s)\\%% User Time", ProcessName.c_str());
	StringCchPrintfW(ProcessNonPagePoolQuery, 256, L"\\Process(%s)\\Pool Nonpaged Bytes", ProcessName.c_str());
	StringCchPrintfW(ProcessUsingMemoryQuery, 256, L"\\Process(%s)\\Private Bytes", ProcessName.c_str());
	
	// PDH 쿼리 핸들 생성 (하나로 모든 데이터 수집 가능)
	PdhOpenQuery(NULL, NULL, &Query);

	//// PDH 리소스 카운터 생성 (여러개 수집시 이를 여러개 생성)
	PdhAddCounter(Query, L"\\Processor(_Total)\\% Processor Time", NULL, &CpuTotal);
	PdhAddCounter(Query, ProcessCpuTotalQuery, NULL, &ProcessCpuTotal);
	PdhAddCounter(Query, ProcessCpuKernelQuery, NULL, &ProcessCpuKernel);
	PdhAddCounter(Query, ProcessCpuUserQuery, NULL, &ProcessCpuUser);
	PdhAddCounter(Query, ProcessNonPagePoolQuery, NULL, &ProcessNonPagePool);
	PdhAddCounter(Query, ProcessUsingMemoryQuery, NULL, &ProcessUsingMemory);
	PdhAddCounter(Query, L"\\Memory\\Available MBytes", NULL, &AvailableMemory);
}
CPerformanceDataHelp::~CPerformanceDataHelp()
{

}
void CPerformanceDataHelp::monitor()
{
	// 데이터 갱신
	PdhCollectQueryData(Query);
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	PDH_FMT_COUNTERVALUE CountValue;
	/*PdhGetFormattedCounterValue(CpuTotal, PDH_FMT_DOUBLE, NULL, &CountValue);
	wprintf(L"CPU : %f%% ", CountValue.doubleValue);
	PdhGetFormattedCounterValue(ProcessCpuTotal, PDH_FMT_DOUBLE, NULL, &CountValue);
	wprintf(L"[Chatting Total:%.0f%% ", CountValue.doubleValue / si.dwNumberOfProcessors);
	PdhGetFormattedCounterValue(ProcessCpuKernel, PDH_FMT_DOUBLE, NULL, &CountValue);
	wprintf(L"K:%.0f%% ", CountValue.doubleValue / si.dwNumberOfProcessors);
	PdhGetFormattedCounterValue(ProcessCpuUser, PDH_FMT_DOUBLE, NULL, &CountValue);
	wprintf(L"U:%.0f%%]\n", CountValue.doubleValue / si.dwNumberOfProcessors);*/
	PdhGetFormattedCounterValue(ProcessNonPagePool, PDH_FMT_LONG, NULL, &CountValue);
	wprintf(L"NonPagePool : %dKByte\n", (long)(CountValue.longValue / 1024));
	PdhGetFormattedCounterValue(ProcessUsingMemory, PDH_FMT_LONG, NULL, &CountValue);
	wprintf(L"UsingMemory : %dMByte\n", (long)(CountValue.longValue / 1024 / 1024));
	//PdhGetFormattedCounterValue(AvailableMemory, PDH_FMT_LONG, NULL, &CountValue);
	//wprintf(L"AvailableMemory		: %dMByte\n", CountValue.longValue);

	return;
}
void CPerformanceDataHelp::GetPDH(PDH& pdh)
{
	PdhCollectQueryData(Query);

	PDH_FMT_COUNTERVALUE CountValue;
	PdhGetFormattedCounterValue(CpuTotal, PDH_FMT_DOUBLE, NULL, &CountValue);
	pdh.CpuTotal = CountValue.doubleValue;
	PdhGetFormattedCounterValue(ProcessCpuTotal, PDH_FMT_DOUBLE, NULL, &CountValue);
	pdh.ProcessCpuTotal = CountValue.doubleValue;
	PdhGetFormattedCounterValue(ProcessCpuKernel, PDH_FMT_DOUBLE, NULL, &CountValue);
	pdh.ProcessCpuKernel = CountValue.doubleValue;
	PdhGetFormattedCounterValue(ProcessCpuUser, PDH_FMT_DOUBLE, NULL, &CountValue);
	pdh.ProcessCpuUser = CountValue.doubleValue;
	PdhGetFormattedCounterValue(ProcessNonPagePool, PDH_FMT_LONG, NULL, &CountValue);
	pdh.ProcessNonPagePool = CountValue.longValue;
	PdhGetFormattedCounterValue(ProcessUsingMemory, PDH_FMT_LONG, NULL, &CountValue);
	pdh.ProcessUsingMemory = CountValue.longValue;
	PdhGetFormattedCounterValue(AvailableMemory, PDH_FMT_LONG, NULL, &CountValue);
	pdh.AvailableMemory = CountValue.longValue;

	return;
}