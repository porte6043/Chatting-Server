#include "Log.h"

// 기타오류 = EtcError
// 시스템 변경 = SYSTEM_INFO

CLog::CLog() : _LogCount(0), _LogLevel(LEVEL_DEBUG), _RootFile(L"Log")
{
    // 로그 기본 Root File 생성
    CreateDirectory(_RootFile, NULL);
}
CLog::~CLog() {}

CLog* CLog::GetInstance()
{
    static CLog Log;
    return &Log;
}

// 현재 시간 설정
void CLog::CurrTime(WCHAR* y, WCHAR* m, WCHAR* d, WCHAR* strTime)
{
    // 현재시간 얻기
    time_t now = time(nullptr);

    // 지역에 맞는 시간으로 변환 
    tm localTime;
    localtime_s(&localTime, &now);

    // 날짜 초기화
    //wcsftime(y, 9, L"%y-%m-%d", &localTime); // 연월일
    wcsftime(y, 3, L"%y", &localTime); // 연월일
    wcsftime(m, 3, L"%m", &localTime); // 연월일
    wcsftime(d, 3, L"%d", &localTime); // 연월일

    // 시간 초기화
    wcsftime(strTime, 9, L"%H:%M:%S", &localTime); // 현재 시간
}

// 로그 파일 경로 설정
void CLog::LogRootFileSet(const WCHAR* fileName)
{
    WCHAR PrevRootFile[ROOTFILE_LEN] = { 0, };
    StringCchCopyW(PrevRootFile, ROOTFILE_LEN, _RootFile);

    LOG(L"SYSTEM_INFO", LEVEL_SYSTEM, L"Try Change RootFile [%s -> %s]", _RootFile, fileName);
    HRESULT Ret = StringCchCopyW(_RootFile, ROOTFILE_LEN, fileName);
    if (FAILED(Ret))
        LOG(L"EtcError", LEVEL_ERROR, L"Lack RootFile Name Buffer");

    LOG(L"SYSTEM_INFO", LEVEL_SYSTEM, L"Changed RootFile [%s -> %s]", PrevRootFile, fileName);

    return;
}

// 로그 레벨 설정
void CLog::LogLevelSet(long logLevel)
{
    InterlockedExchange(&_LogLevel, logLevel);
    LOG(L"SYSTEM_INFO", LEVEL_SYSTEM, L"Change LogLevel");
    return;
}

// Logging
void CLog::Log(const WCHAR* logCategory, int logLevel, const WCHAR* stringFormat, ...)
{
    // _LogLevel보다 크거나 같을 때만 Logging
    if (_LogLevel > logLevel)
        return;

    const WCHAR* Level[3] = { L"DEBUG", L"ERROR", L"SYSTEM" };
    WCHAR Time[9] = { 0, };
    WCHAR Y[3] = { 0, };
    WCHAR M[3] = { 0, };
    WCHAR D[3] = { 0, };
    WCHAR Path[PATH_LEN] = { 0, };
    WCHAR Message[MESSAGE_LEN] = { 0, };

    // 현재 날짜와 시간 초기화
    CurrTime(Y, M, D, Time);

    // txt경로 초기화
    HRESULT Path_Ret = StringCchPrintfW(Path, PATH_LEN, L"./%s/20%s/%s월/%s/20%s%s_%s.txt", _RootFile, Y, M, logCategory, Y, M, logCategory);
    if (FAILED(Path_Ret))
    {
        LOG(L"EtcError", LEVEL_ERROR, L"Lack LogPath Buffer");
        return;
    }

    // String Formet 
    va_list va;
    va_start(va, stringFormat);
    HRESULT Formet_Ret = StringCchVPrintfW(Message, MESSAGE_LEN, stringFormat, va);
    if (FAILED(Formet_Ret))
    {
        LOG(L"EtcError", LEVEL_ERROR, L"Lack Message Buffer");
    }
    va_end(va);


    // TXT 저장
    FILE* pFile;
    if (_wfopen_s(&pFile, Path, L"ab") != 0)
    {
        // 먼저 폴더 생성 부터 해봄
        CreateFolder(Path);

        // 열릴 때 까지 계속 시도
        while (_wfopen_s(&pFile, Path, L"ab")) {} 
    }

    InterlockedIncrement(&_LogCount);
    fwprintf(pFile, L"[%s] [20%s-%s-%s %s / %s / %010u] %s\n",
        logCategory, Y, M, D, Time, Level[logLevel], _LogCount, Message);

    fclose(pFile);

    return;
}

void CLog::CreateFolder(WCHAR* path)
{
    WCHAR TempPath[PATH_LEN] = { 0, };

    // 파일 경로에서 마지막 디렉토리 이름 추출
    WCHAR* LastSlash = wcsrchr(path, L'/');
    WCHAR StrTemp;

    if (LastSlash != nullptr) {

        // 마지막 디렉토리 이름을 제거하여 디렉토리 경로만 남김
        StrTemp = *(LastSlash + 1);
        *(LastSlash + 1) = L'\0';

        int strlen = 0;
        for (int iCnt = 0; iCnt < PATH_LEN; ++iCnt)
        {
            if (path[iCnt] == '/')
            {
                for (int jCnt = iCnt + 1; jCnt < PATH_LEN - iCnt; ++jCnt)
                {
                    if (path[jCnt] == '/')
                    {
                        strlen = jCnt;
                        break;
                    }
                }

                memcpy(TempPath, path, sizeof(WCHAR) * strlen);
                CreateDirectory(TempPath, nullptr);
            }
        }


        *(LastSlash + 1) = StrTemp;  // 원래 경로로 복원
    }

    return;
}
