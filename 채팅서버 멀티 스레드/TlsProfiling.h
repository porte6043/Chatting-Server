#include <stdio.h>
#include <iostream>
#include <windows.h>
#pragma comment(lib, "Winmm.lib")

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
// 맨 처음에 #define PROFILE 를 항상 해준다
// 
// PRO_BEGIN(L"tagname");
// profile할 코드..
// PRO_END(L"tagname");
// 
// 생성자 소멸자을 이용한 Profile
// CTlsProfile Profile(L"tagname"); 
// 
// 
// txt파일로 출력 하고 싶을 시 ProfileDataOutText(L"txt.파일이름");
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-


// 클래스 형태
#ifndef __TLSPROFILING__
#define __TLSPROFILING__

#define PROFILE
#ifdef PROFILE
#define PRO_BEGIN(TagName) CTlsProfiling::GetInstance()->ProfileBegin(TagName)
#define PRO_END(TagName) CTlsProfiling::GetInstance()->ProfileEnd(TagName)
#else
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif

#define TAGARRCOUNT 25
#define THREADCOUNT 20

class CTlsProfiling
{
private:
	struct TAG_NAME
	{
		bool _Flag;					// 프로파일의 사용 여부(배열 사용 시)
		WCHAR _TagName[64];			// 프로파일 Tag 이름
		LARGE_INTEGER _StartTime;	// 프로파일 실행 시간
		DWORD64 _TotalTime;			// 전체 사용시간 (출력시 호출회수:Call로 나누어 평균을 구함)
		DWORD64	_Min[3];			// 최소 실행 시간 (0->1->2 작은 숫자)
		DWORD64 _Max[3];			// 최대 실행 시간 (0->1->2 큰 숫자)
		DWORD64 _Call;				// 호출 횟수
	};

private:
	TAG_NAME TAG[THREADCOUNT][TAGARRCOUNT];
	DWORD ThreadID[THREADCOUNT];
	DWORD TlsIndex;					// Tls 인덱스의 값
	DWORD TagNameIndex;				// TAG_NAME 배열의 인덱스 값


private: CTlsProfiling();
private: ~CTlsProfiling();

private: void ProfileReset();

private: void AlignText_fwprintf_s(FILE* file, const WCHAR* tagname, float average, float min, float max, __int64 call);

public: static CTlsProfiling* GetInstance();

public: void ProfileBegin(const WCHAR* tagname);

public: void ProfileEnd(const WCHAR* tagname);

public: void ProfileDataOutText(const WCHAR* TextFileName);
};

class CTlsProfile
{
friend class CTlsProfiling;  
private:
	const WCHAR* TagName;

public:
	CTlsProfile(const WCHAR* tagname)
	{ 
		TagName = tagname;
		PRO_BEGIN(TagName);
	}
	~CTlsProfile()
	{
		PRO_END(TagName);
	}
};

#endif  