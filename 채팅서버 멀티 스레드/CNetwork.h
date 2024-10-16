#ifndef __NETWORK__
#define __NETWORK__
#define SESSIONID(SessionID, SessionIndex) ( ((DWORD64)SessionID << 32) | (DWORD64)SessionIndex )
#define SESSIONID_ID_HIGH(SessionID) (DWORD)(SessionID >> 32)
#define SESSIONID_INDEX_LOW(SessionID) (DWORD)(SessionID & 0x0000'0000'ffff'ffff)

#define IOCOUNT(Flag, IOCount) ( ((DWORD)Flag << 16) |  (DWORD)IOCount )
#define IOCOUNT_FLAG_HIGH(IOCount) (DWORD)(IOCount >> 16)
#define IOCOUNT_IOCOUNT_LOW(IOCount) (DWORD)(IOCount & 0x0000'ffff)

#define INVALID_SESSIONID 0
#define df_PACKET_CODE 0x77


#pragma comment(lib, "ws2_32") // WinSock2.h 라이브러리
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <unordered_map>
#include <queue>
#include <stack>
#include <list>
#include <Windows.h>
using namespace std;

#define PROFILE
#include "TlsProfiling.h"
#include "SerialRingBuffer.h"
#include "PerformanceDataHelp.h"
#include "CpuUsage.h"
#include "TextPasing.h"
#include "Log.h"
#include "Monitor.h"

class CNETWORK
{
#pragma pack(1) 
	struct Network_Header
	{
		BYTE Code;
		WORD Len;
		BYTE RandKey;
		BYTE CheckSum;
		Network_Header()
		{
			Code = 0;
			Len = 0;
			RandKey = 0;
			CheckSum = 0;
		}
		Network_Header(SerialRingBuffer* packet)
		{
			Code = df_PACKET_CODE;
			Len = packet->GetUseSize();
			RandKey = rand() % 256;
			CheckSum = 0;

			char* byte = packet->front();
			for (int iCnt = 0; iCnt < Len; ++iCnt)
			{
				CheckSum += *(byte + iCnt);
			}
		}
	};
#pragma pack()

public:
	struct Session
	{
		SOCKET socket;
		SerialRingBuffer RecvQ;
		SerialRingBuffer SendQ;
		OVERLAPPED RecvOverlapped;
		OVERLAPPED SendOverlapped;
		DWORD64 SessionID;		//  High 4Byte : SessionID , Low 4Byte : Sessions Index
		DWORD SendFlag;			// '0': Not Sending , '1': Sending
		DWORD DisconnectFlag;
		DWORD IOCount;
		DWORD SendBufferCount;
		CRITICAL_SECTION CS_SendQ;


		Session()
		{
			socket = NULL;
			SendFlag = 0;
			DisconnectFlag = 0;
			SessionID = INVALID_SESSIONID;
			IOCount = 0;
			SendBufferCount = 0;
			ZeroMemory(&RecvOverlapped, sizeof(RecvOverlapped));
			ZeroMemory(&SendOverlapped, sizeof(SendOverlapped));
			InitializeCriticalSection(&CS_SendQ);
		}
	};

public:
	struct PerformanceNetworkData
	{
		DWORD SessionCount;			// GetSessionCount()
		DWORD AcceptTotalCount;		// 
		DWORD PacketTotalCount;		// SerialRingBuffer::GetPoolTotalCount();
		DWORD PacketUseCount;		// SerialRingBuffer::GetPoolUseCount();
		DWORD AcceptTPS;			//
		DWORD SendTPS;				//
		DWORD RecvTPS;				//

		PerformanceNetworkData() :
			SessionCount(0),
			AcceptTotalCount(0),
			PacketTotalCount(0),
			PacketUseCount(0),
			AcceptTPS(0),
			SendTPS(0),
			RecvTPS(0) {}
	};

private:
	struct Timer
	{
		void (*TimerFunction)(LPVOID);
		LARGE_INTEGER Time;
		LPVOID p;
	};


private:
	Session* Sessions;
	stack<DWORD> SessionIndex;
	CRITICAL_SECTION CS_SessionIndex;
	HANDLE hTimers[MAXIMUM_WAIT_OBJECTS];
	Timer TimerList[MAXIMUM_WAIT_OBJECTS];
	long TimerCount;


	SOCKET ListenSocket;		// 리슨 소켓 ( AcceptThread를 종료할 떄 사용합니다.)
	WORD Port;					// Server Port
	WORD MaxSessionCount;		// 최대 Session 수
	WORD NumberOfCore;			// WorkerThread의 수
	HANDLE hCP;					// IOCP 핸들
	HANDLE hAccept_Threads;		// Accept ID
	HANDLE hTimer_Threads;		// Timer ID
	HANDLE* hWorker_Threads;	// 워커스레드 ID
	PerformanceNetworkData Pnd;	// 모니터용 변수
public:
	CPerformanceDataHelp Pdh;	// 모니터용 변수 (하드웨어)



public:
	CNETWORK();
	~CNETWORK();

	// 화이트 IP와 Port 접속 여부를 판단합니다. (Accpet 후)
	virtual bool OnAcceptRequest(WCHAR* IP, WORD Port) = 0;

	// Accept 후 Session이 생성된 후 컨텐츠에 Client의 Session 생성을 알립니다.
	virtual void OnSessionConnected(DWORD64 SessionID) = 0;

	// Session이 삭제 된 후 호출합니다.
	virtual void OnSessionRelease(DWORD64 SessionID) = 0;

	// Message 수신 완료 후 Contents Packet 처리를 합니다.
	//virtual void OnRecv(DWORD64 SessionID, SerialPacket* packet) = 0;
	virtual void OnRecv(DWORD64 SessionID, CPacket& packet) = 0;

public:
	bool Start(const WCHAR* IP, WORD Port, WORD NumberOfCreateThreads, WORD NumberOfConcurrentThreads, bool Nagle, WORD MaxSessionCount);

	bool Disconnect(DWORD64 SessionID);
private: void Disconnect(Session* pSession);

public:
	bool SendMSG(DWORD64 SessionID, CPacket& packet);

	int GetSessionCount();

	void GetPND(PerformanceNetworkData& pnd);

	int GetPort();

	int GetAcceptTPS();

	int GetRecvTPS();

	int GetSendTPS();

	int GetTotalAcceptCount();

	int GetTotalDisconnectCount();

	void RegisterTimer(void (*TimerFunction)(LPVOID), LPVOID p, DWORD dwMilliseconds);
	//private:
public:
	bool ListenSocketInit(const WCHAR* IP, WORD Port, bool Nagle);

	// 세션의 IP와 Port를 얻어옵니다.
	void ClientInfo(Session* pSession, WCHAR* IP, WORD* Port);
	void ClientInfo(DWORD64 SessionID, WCHAR* IP, WORD* Port);

	// 네트워크의 에러가 발생할 시 로깅을 위한 함수입니다.
	void NetworkError(int Err, const WCHAR* FunctionName);

	// 네트워크 메시지의 헤더 확인해서 컨텐츠 Paload 추출 및 전달합니다.
	void NetworkMessageProc(Session* pSession);

	// WSARecv 보내기
	void RecvPost(Session* pSession);

	//  WSASend 보내기
	void SendPost(Session* pSession);

	// Session 삭제
	void SessionRelease(Session* pSession);

	Session* SessionFind(DWORD64 SessionID);

	// 새로운 세션 할당을 위해서 사용 중이 않는 세션을 찾습니다.
	Session* SessionAlloc(SOCKET Socket, DWORD64 sessionID);

	// 해제된 세션ID을 반환합니다.
	DWORD64 SessionFree(Session* pSession);

	// 출력창에 출력합니다.
	void OutputDebugStringF(LPCTSTR format, ...);

	static void UpdatePND(LPVOID p);

	// 함수 내부에서 일부로 크러쉬를 냅니다. 발생 하면 코드의 결함입니다.
	void Crash();

	static unsigned WINAPI WorkerThreadWrapping(LPVOID p);
	void WorkerThread();

	static unsigned WINAPI TimerThreadWrapping(LPVOID p);
	void TimerThread();

	static unsigned WINAPI AcceptThreadWrapping(LPVOID p);
	void AcceptThread();
};




#endif
