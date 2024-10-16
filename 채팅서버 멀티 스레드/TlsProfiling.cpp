#include "TLSProfiling.h"



CTlsProfiling::CTlsProfiling() : TAG{}, ThreadID{}, TagNameIndex(-1)
{
	timeBeginPeriod(1);

	TlsIndex = TlsAlloc();
	if (TlsIndex == TLS_OUT_OF_INDEXES)
	{
		printf("Tls Lack...\n");
		*(int*)0 = 0;
	}
}

CTlsProfiling::~CTlsProfiling()
{
	timeEndPeriod(0);
}

CTlsProfiling* CTlsProfiling::GetInstance()
{
	static CTlsProfiling TlsProfiling;
	return &TlsProfiling;
}


void CTlsProfiling::ProfileBegin(const WCHAR* tagname)
{
	TAG_NAME* Tag = (TAG_NAME*)TlsGetValue(TlsIndex);
	if (Tag == NULL)
	{
		DWORD TagIndex = InterlockedIncrement(&TagNameIndex);
		TlsSetValue(TlsIndex, &TAG[TagIndex][0]);
		ThreadID[TagIndex] = GetCurrentThreadId();
		Tag = &TAG[TagIndex][0];
	}

	// tagname에 해당하는 Sample이 있는지 확인
	for (int Index = 0; Index < TAGARRCOUNT; ++Index)
	{
		if (wcscmp(Tag[Index]._TagName, tagname) == 0)
		{
			QueryPerformanceCounter(&Tag[Index]._StartTime);
			return;
		}
	}

	// Sample 생성
	for (int Index = 0; Index < TAGARRCOUNT; ++Index)
	{
		// 비어있는 Index 찾기
		if (Tag[Index]._Flag != 0) continue;

		// ProfileSample 초기화
		Tag[Index]._Flag = 1;
		for (int iMin = 0; iMin < 3; ++iMin)
		{
			Tag[Index]._Min[iMin] = 0x7fffffffffffffff;
		}
		wcscpy_s(Tag[Index]._TagName, 64, tagname);
		QueryPerformanceCounter(&Tag[Index]._StartTime);

		return;
	}

	printf("TAG_NAME Lack...\n");
	*(int*)0 = 0;
}

void CTlsProfiling::ProfileEnd(const WCHAR* tagname)
{
	LARGE_INTEGER EndTime;
	__int64 DeltaTime;

	// 조금이라도 정확히 측정하기 위해 tagname이 있던 없던 종료 시간 먼저 저장
	QueryPerformanceCounter(&EndTime);

	TAG_NAME* Tag = (TAG_NAME*)TlsGetValue(TlsIndex);

	// tagname에 해당하는 Sample이 있는지 확인
	int Index = 0;
	for (; Index < TAGARRCOUNT; ++Index)
	{
		if (wcscmp(Tag[Index]._TagName, tagname) != 0)
			continue;
		else
			break;
	}

	// tagname에 해당하는 Sample이 있으면 아래 로직처리
	DeltaTime = EndTime.QuadPart - Tag[Index]._StartTime.QuadPart;
	Tag[Index]._TotalTime += DeltaTime;


	// _Min에서 가장 긴 실행 시간 찾아서 변경
	int TempMaxIndex = 0;						// _Min에서 가장 긴 실행 시간의 인덱스
	__int64 TempMin = 0;
	for (int iCt = 0; iCt < 3; ++iCt)
	{
		if (Tag[Index]._Min[iCt] > TempMin)
		{
			TempMaxIndex = iCt;
			TempMin = Tag[Index]._Min[iCt];
		}
	}
	if (Tag[Index]._Min[TempMaxIndex] > DeltaTime)
	{
		Tag[Index]._Min[TempMaxIndex] = DeltaTime;
	}


	// _Max에서 가장 작은 실행 시간 찾아서 변경
	int TempMinIndex = 0;						// _Max에서 가장 짧은 실행 시간의 인덱스
	__int64 TempMax = Tag[Index]._Max[0];	// 첫 비교는 _Max[0]
	for (int iCt = 0; iCt < 3; ++iCt)
	{
		if (Tag[Index]._Max[iCt] < TempMax)
		{
			TempMaxIndex = iCt;
			TempMax = Tag[Index]._Max[iCt];
		}
	}
	if (Tag[Index]._Max[TempMaxIndex] < DeltaTime)
	{
		Tag[Index]._Max[TempMaxIndex] = DeltaTime;
	}

	++Tag[Index]._Call;

	return;
}

void CTlsProfiling::ProfileDataOutText(const WCHAR* TextFileName)
{
	float Average;
	float Min;
	float Max;
	float Sum = 0;

	FILE* pFile;
	while (_wfopen_s(&pFile, TextFileName, L"w, ccs=UNICODE") != 0) {}

	for (int ThreadIndex = 0; ThreadIndex < TagNameIndex + 1; ++ThreadIndex)
	{
		// txt파일에 쓰기
		fwprintf_s(pFile, L"%-24s|%24s|%24s|%24s|%24s|\n", L"Name", L"Average", L"Min", L"Max", L"Call");
		fwprintf_s(pFile, L"TheadID:%d----------------------------------------------------------------------------------------------------------------\n", ThreadID[ThreadIndex]);
		for (int TagIndex = 0; TagIndex < TAGARRCOUNT; ++TagIndex)
		{
			// _Flag가 0일 때 패스
			if (TAG[ThreadIndex][TagIndex]._Flag != 1) continue;

			// Average 구하기
			Sum = 0;
			for (int i = 0; i < 3; ++i)
			{
				Sum += TAG[ThreadIndex][TagIndex]._Min[i];
				Sum += TAG[ThreadIndex][TagIndex]._Max[i];
			}
			Average = (float)(TAG[ThreadIndex][TagIndex]._TotalTime - Sum) / (TAG[ThreadIndex][TagIndex]._Call - 6) / 10;	 // 마이크로 단위

			// Min 구하기
			Min = TAG[ThreadIndex][TagIndex]._Min[0];
			for (int i = 0; i < 3; ++i)
			{
				if (TAG[ThreadIndex][TagIndex]._Min[i] < Min)
				{
					Min = (float)TAG[ThreadIndex][TagIndex]._Min[i] / 10;		// 마이크로 단위
				}
			}

			// Max 구하기
			Max = TAG[ThreadIndex][TagIndex]._Max[0];
			for (int i = 0; i < 3; ++i)
			{
				if (TAG[ThreadIndex][TagIndex]._Max[i] < Max)
				{
					Max = (float)TAG[ThreadIndex][TagIndex]._Max[i] / 10;		// 마이크로 단위
				}
			}

			AlignText_fwprintf_s(pFile, TAG[ThreadIndex][TagIndex]._TagName, Average, Min, Max, TAG[ThreadIndex][TagIndex]._Call);
		}
		fwprintf_s(pFile, L"----------------------------------------------------------------------------------------------------------------------------\n\n");
	}

	fclose(pFile);
}

void CTlsProfiling::ProfileReset()
{

}

void CTlsProfiling::AlignText_fwprintf_s(FILE* file, const WCHAR* tagname, float average, float min, float max, __int64 call)
{
	fwprintf_s(file, L"%-24s|", tagname);
	fwprintf_s(file, L"%22.4f%s|", average, L"\u00B5s");
	fwprintf_s(file, L"%22.4f%s|", min, L"\u00B5s");
	fwprintf_s(file, L"%22.4f%s|", max, L"\u00B5s");
	fwprintf_s(file, L"%24lld|\n", call);

	return;
}