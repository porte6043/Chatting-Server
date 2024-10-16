#ifndef __PERFOEMANCE_DATA_HELP__
#define __PERFOEMANCE_DATA_HELP__
#include <Windows.h>
#include <strsafe.h>
#include <string>
#include <Pdh.h>
#pragma comment(lib,"Pdh.lib")

class CPerformanceDataHelp
{
public:
	struct PDH
	{
		float CpuTotal;
		float ProcessCpuTotal;
		float ProcessCpuKernel;
		float ProcessCpuUser;
		long ProcessNonPagePool;
		long ProcessUsingMemory;
		long AvailableMemory;

		PDH()
		{
			CpuTotal = 0;
			ProcessCpuTotal = 0;
			ProcessCpuKernel = 0;
			ProcessCpuUser = 0;
			ProcessNonPagePool = 0;
			ProcessUsingMemory = 0;
			AvailableMemory = 0;
		}
	};
private:
	PDH Pdh;

	// PDH 쿼리 핸들 생성 (하나로 모든 데이터 수집 가능)
	PDH_HQUERY Query;
	//// PDH 리소스 카운터 생성 (여러개 수집시 이를 여러개 생성)
	PDH_HCOUNTER CpuTotal;
	PDH_HCOUNTER ProcessCpuTotal;
	PDH_HCOUNTER ProcessCpuKernel;
	PDH_HCOUNTER ProcessCpuUser;
	PDH_HCOUNTER ProcessNonPagePool;
	PDH_HCOUNTER ProcessUsingMemory;
	PDH_HCOUNTER AvailableMemory;

	WCHAR ProcessCpuTotalQuery[256];
	WCHAR ProcessCpuKernelQuery[256];
	WCHAR ProcessCpuUserQuery[256];
	WCHAR ProcessNonPagePoolQuery[256];
	WCHAR ProcessUsingMemoryQuery[256];

public: CPerformanceDataHelp();
public: ~CPerformanceDataHelp();

public: void monitor();
public: void GetPDH(PDH& pdh);

};


#endif