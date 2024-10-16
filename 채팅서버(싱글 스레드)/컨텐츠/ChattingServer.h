#ifndef  __CHATTINGSERVER__
#define __CHATTINGSERVER__

#include <unordered_set>
#include "네트워크/CWanServer.h"
#include "st_ChatServer.h"
#include "ChattingProtocol.h"

#include "MonitoringClient_ChattingServer.h"


#define INVALID_ACCOUNTNO (INT64)-1

class ChattingServer : public CWanServer
{
public:
	struct PerformanceContentsData
	{
		DWORD UpdateMessage_Queue;		// UpdateThread 큐 남은 개수 (MessageQ.size)
		DWORD User_Pool;				// User 구조체 할당량 (UserPool.Size = GuestMap.Size + UserMap.Size)
		DWORD UserCount;				// 로그인 후 User 개수 (UserMap.UseSize)
		DWORD UpdateTPS;				// 초당 Update 처리 횟수 (Message_Packet + Message_Accept + Message_Disconnect)
		DWORD Message_PacketTPS;		// Message_Packet (Message_Packet)
		DWORD Message_AcceptTPS;		// Message_Accept (Message_Accept)
		DWORD Message_DisconnectTPS;	// Message_Disconnect (Message_Disconnect)
		DWORD Packet_LoginTPS;			// Packet_Req_Login
		DWORD Packet_SectorMoveTPS;		// Packet_Req_SectorMove
		DWORD Packet_MessageTPS;		// Packet_Req_Message
		DWORD Packet_HeartBeatTPS;		// Packet_Req_HeartBeat
		DWORD DuplicateLoginTotalCount; // 중복 로그인 총 카운트

		PerformanceContentsData()
		{
			UpdateMessage_Queue = 0;
			User_Pool = 0;
			UserCount = 0;
			UpdateTPS = 0;
			Message_PacketTPS = 0;
			Message_AcceptTPS = 0;
			Message_DisconnectTPS = 0;
			Packet_LoginTPS = 0;
			Packet_SectorMoveTPS = 0;
			Packet_MessageTPS = 0;
			Packet_HeartBeatTPS = 0;
			DuplicateLoginTotalCount = 0;
		}
			
	};

private:
	HANDLE hContents_Thread;										// Contents 스레드

	queue<Message> MessageQueue;
	queue<Message> WorkingQueue;
	queue<Message>* MessageQ;
	queue<Message>* WorkingQ;
	CRITICAL_SECTION CS_MessageQ;
	HANDLE hEvent;

	CTlsPool<ChatUser> UserPool;									// 유저 풀
	unordered_set<INT64> AccountSet;								// 로그인한 유저의 중복 확인 체크 // 나중에 Map으로 변경
	unordered_set<DWORD64> AccountSet_Flag;
	unordered_map<DWORD64, ChatUser*> UserMap;						// 로그인 후 유저맵
	unordered_map<DWORD64, ChatUser*> GuestMap;						// 로그인 전 유저맵
	Sector Sectors[en_SECTOR_MAXIMUM_Y][en_SECTOR_MAXIMUM_X];		// 섹터

	CCpuUsage CpuUsage;												// 하드웨어 데이터
	PerformanceContentsData Pcd;									// 모니터링 데이터

	CMonitoringClient_Chatting MonitorClient;						// 모니터링 클라이언트

public:
	// 서버 세팅
	// Network
	WCHAR IP[16];
	WORD Port;
	WORD WorkerThread;
	WORD ActiveThread;
	bool Nagle;
	bool ZeroCopySend;
	WORD SessionMax;
	BYTE PacketCode;
	BYTE PacketKey;

	// Contents
	WORD UserMax;
	DWORD UserHeartBeat;
	DWORD GuestHeartBeat;
	bool Heartbeat;

	// System
	BYTE LogLevel;


private:
	static unsigned WINAPI ContentsThreadWrapping(LPVOID p);
	void ContentsThread();

	// 화이트 IP와 Port 접속 여부를 판단합니다. (Accpet 후)
	virtual bool OnAcceptRequest(WCHAR* IP, WORD Port);

	// Accept 후 Session이 생성된 후 컨텐츠에 Client의 Session 생성을 알립니다.
	virtual void OnSessionConnected(DWORD64 SessionID);

	// Session이 삭제 된 후 호출합니다.
	virtual void OnSessionRelease(DWORD64 SessionID);

	// Message 수신 완료 후 Contents Packet 처리를 합니다.
	virtual void OnRecv(DWORD64 SessionID, CPacket& packet);

	ChatUser* FindUser(DWORD64 SessionID);

	ChatUser* FindGuest(DWORD64 SessionID);

	void MessagePush(Message& message);

	bool isDuplicateLogin(INT64 AccountNo);

	bool isValidToken(const char* sessionKey);

	// 반환값 다르면 false 같으면 true
	bool CompareAccountNo(ChatUser* user, INT64 AccountNo);

	void SendToMonitoringServer(PerformanceHardwareData& Phd, PerformanceNetworkData& Pnd, PerformanceContentsData& Pcd);

	static void GuestHeartbeat(LPVOID p);

	static void UserHeartbeat(LPVOID p);



	// Chat_Sector.cpp
	void Sector_GetAround(ChatUser* user, SectorAround& sectorAround);
	void Sector_GetAround(WORD x, WORD y, SectorAround& sectorAround);
	void Sector_UserErase(ChatUser* user);
	bool Sector_Update(ChatUser* user, WORD SectorX, WORD SectorY);
	void Sector_SendAround(ChatUser* user, CPacket& packet, bool sendMe);
	void Sector_SendOne(ChatUser* user, CPacket& packet);

	// Chat_MessageProc.cpp
	void Proc_Accept(DWORD64 SessionID, CPacket& packet);
	void Proc_Disconnect(DWORD64 SessionID, CPacket& packet);
	void Proc_Packet(DWORD64 SessionID, CPacket& packet);
	void Proc_GuestHeartbeat();
	void Proc_UserHeartbeat();

	// Chat_Packet.cpp
	void Packet_Req_Login(DWORD64 SessionID, CPacket& packet);			// 채팅서버 로그인 요청
	void Packet_Req_Sector_Move(DWORD64 SessionID, CPacket& packet);	// 채팅서버 섹터 이동 요청
	void Packet_Req_Message(DWORD64 SessionID, CPacket& packet);		// 채팅서버 채팅보내기 요청
	void Packet_Req_HeartBeat(DWORD64 SessionID, CPacket& packet);		// 채팅서버 클라이언트 하트비트

	// Chat_CreateMessage.cpp
	void InitPacket_ResLogin(CPacket& packet, BYTE Status, INT64 AccountNo);
	void InitPacket_ResSectorMove(CPacket& packet, INT64 AccountNo, WORD SectorX, WORD SectorY);
	void InitPacket_ResMessage(CPacket& packet, INT64 AccountNo, const WCHAR* ID, const WCHAR* Nickname, WORD MessageLen, const WCHAR* Message);


public:
	ChattingServer();

	// 모니터링 함수
	void monitor();

	// 섹터별 인원 수 출력
	void SectorUserTextOut();
	void Proc_SectorUserTextOut();

	void GetPCD(PerformanceContentsData& pcd);
};
#endif