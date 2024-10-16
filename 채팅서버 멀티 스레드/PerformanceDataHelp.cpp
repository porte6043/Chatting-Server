#include "PerformanceDataHelp.h"


CPerformanceDataHelp::CPerformanceDataHelp() : 
	ProcessCpuTotalQuery{}, ProcessCpuKernelQuery{},
	ProcessCpuUserQuery{}, ProcessNonPagePoolQuery{},
	ProcessUsingMemoryQuery{}
{
	WCHAR buf[256] = { 0, };
	DWORD bufLen = sizeof(buf);
	DWORD pid = GetCurrentProcessId();
	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);	// ���μ��� pid�� ���μ��� �ڵ� �˻�
	QueryFullProcessImageName(hProc, 0, buf, &bufLen);							// ���μ��� �ڵ��� ���� ��ü ��� ã��
	CloseHandle(hProc);

	// ���� ��ο��� ���丮 �����ڸ� ã��
	std::wstring ProcessPath = buf;
	size_t StartIndex = ProcessPath.find_last_of(L"\\/");
	size_t LastIndex = ProcessPath.find_last_of(L".");

	// ���μ��� �̸�
	std::wstring ProcessName = ProcessPath.substr(StartIndex + 1, LastIndex - StartIndex - 1);

	// ������ �Է�
	StringCchPrintfW(ProcessCpuTotalQuery, 256, L"\\Process(%s)\\%% Processor Time", ProcessName.c_str());
	StringCchPrintfW(ProcessCpuKernelQuery, 256, L"\\Process(%s)\\%% Privileged Time", ProcessName.c_str());
	StringCchPrintfW(ProcessCpuUserQuery, 256, L"\\Process(%s)\\%% User Time", ProcessName.c_str());
	StringCchPrintfW(ProcessNonPagePoolQuery, 256, L"\\Process(%s)\\Pool Nonpaged Bytes", ProcessName.c_str());
	StringCchPrintfW(ProcessUsingMemoryQuery, 256, L"\\Process(%s)\\Private Bytes", ProcessName.c_str());
	
	// PDH ���� �ڵ� ���� (�ϳ��� ��� ������ ���� ����)
	PdhOpenQuery(NULL, NULL, &Query);

	//// PDH ���ҽ� ī���� ���� (������ ������ �̸� ������ ����)
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
	// ������ ����
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