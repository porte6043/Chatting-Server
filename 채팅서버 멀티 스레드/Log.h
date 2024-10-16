#include <iostream>
#include <ctime>
#include <strsafe.h>
#include <Windows.h>

#ifndef __LOG__
#define __LOG__

#define LOG(category, level, stringFormat, ...)                                 \
do{                                                                             \
    CLog::GetInstance()->Log(category, level, stringFormat, __VA_ARGS__);       \
}while(0)                                                             

// 로그의 Root File 설정 (기본 Log 파일에 저장) 
#define LOG_DIRECTORY(FILENAME)                                      \
do{                                                                  \
    CLog::GetInstance()->LogRootFileSet(FILENAME);                   \
}while(0)                                                             

// 로그의 Log Level 설정 
#define LOG_LEVEL(LOGLEVEL)                                     \
do{                                                             \
    CLog::GetInstance()->LogLevelSet(LOGLEVEL);                 \
}while(0)                           



class CLog
{
public:
    // SingleTon 로그 클래스
    static CLog* GetInstance();

    enum LOG_LEVER
    {
        LEVEL_DEBUG,
        LEVEL_ERROR,
        LEVEL_SYSTEM
    };

private:
    enum LOG_VALUE
    {
        PATH_LEN = 256,
        MESSAGE_LEN = 256,
        ROOTFILE_LEN = 128
    };

private:
    DWORD _LogCount;
    long _LogLevel;
    WCHAR _RootFile[ROOTFILE_LEN];

private:
    CLog();
    ~CLog();


    // 시스템 로그
public:
    // 로그 파일 경로 설정
    void LogRootFileSet(const WCHAR* logRootFileName);

    // 로그 레벨 설정
    void LogLevelSet(long logLevel);

    // Logging
    void Log(const WCHAR* logCategory, int logLevel, const WCHAR* stringFormat, ...);

    // 시스템 로그
private:
    void CreateFolder(WCHAR* path);

    // strdate len = 9 , strtime len = 9
    void CurrTime(WCHAR* y, WCHAR* m, WCHAR* d, WCHAR* strTime);
};



#endif