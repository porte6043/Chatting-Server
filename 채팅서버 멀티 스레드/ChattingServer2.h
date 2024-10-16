#ifndef  __CHATTINGSERVER__
#define __CHATTINGSERVER__

#include <set>
#include "CNetwork.h"
#include "st_ChatServer.h"
#include "ChattingProtocol.h"
#include "CTlsPool.h"


class ChattingServer2 : public CNETWORK
{
public:
	struct PerformanceContentsData
	{
		DWORD UserCount;				// 로그인 후 User 개수 (UserMap.UseSize)
		DWORD UpdateTPS;				// 초당 Update 처리 횟수 (Message_Packet + Message_Accept + Message_Disconnect)
		DWORD Message_PacketTPS;		// Message_Packet (Message_Packet)
		DWORD Message_AcceptTPS;		// Message_Accept (Message_Accept)
		DWORD Message_DisconnectTPS;	// Message_Disconnect (Message_Disconnect)
		DWORD Packet_LoginTPS;			// Packet_Req_Login
		DWORD Packet_SectorMoveTPS;		// Packet_Req_SectorMove
		DWORD Packet_MessageTPS;		// Packet_Req_Message
		DWORD Packet_HeartBeatTPS;		// Packet_Req_HeartBeat

		PerformanceContentsData()
		{
			UserCount = 0;
			UpdateTPS = 0;
			Message_PacketTPS = 0;
			Message_AcceptTPS = 0;
			Message_DisconnectTPS = 0;
			Packet_LoginTPS = 0;
			Packet_SectorMoveTPS = 0;
			Packet_MessageTPS = 0;
			Packet_HeartBeatTPS = 0;
		}

	};

private:
	unordered_map<DWORD64, ChatUser*> UserMap;				// 유저 배열의 검색 맵
	CRITICAL_SECTION CS_UserMap;

	set<INT64> LoginUserAccountSet;
	CRITICAL_SECTION CS_LoginUserAccountSet;

	CTlsPool<ChatUser> UserPool;

	Sectors sectors;
	PerformanceContentsData Pcd;									// 모니터링 데이터

public:
	// 서버 세팅
	WCHAR IP[16];
	WORD Port;
	WORD WorkerThread;
	WORD ActiveThread;
	bool Nagle;
	WORD SessionMax;
	WORD UserMax;
	WORD PacketCode;
	WORD PacketKey;
	WORD LogLevel;
	DWORD UserHeartBeat;
	DWORD GuestHeartBeat;




private:
	// 화이트 IP와 Port 접속 여부를 판단합니다. (Accpet 후)
	virtual bool OnAcceptRequest(WCHAR* IP, WORD Port);
	// Accept 후 Session이 생성된 후 컨텐츠에 Client의 Session 생성을 알립니다.
	virtual void OnSessionConnected(DWORD64 SessionID);
	// Session이 삭제 된 후 호출합니다.
	virtual void OnSessionRelease(DWORD64 SessionID);
	// Message 수신 완료 후 Contents Packet 처리를 합니다.
	virtual void OnRecv(DWORD64 SessionID, CPacket& packet);

	ChatUser* UserFind(DWORD64 SessionID);

	bool isDuplicateLogin(INT64 AccountNo);
	bool isValidToken(const char* sessionKey);

	// 반환값 다르면 false 같으면 true
	bool CompareAccountNo(ChatUser* user, INT64 AccountNo);

	void Crash();

	static void GuestHeartbeat(LPVOID p);
	static void UserHeartbeat(LPVOID p);



	// Chat_Sector.cpp
	void Sector_GetAround(WORD x, WORD y, SectorAround& sectorAround);
	bool Sector_Update(ChatUser* user, const SectorPos& updatePos);
	void Sector_Sort(const SectorPos& pos1, const SectorPos& pos2, SectorAround& seqSector);
	void Sector_Sort(SectorAround& seqSector);
	void Sector_Lock(const SectorAround& sectorAround);
	void Sector_UnLock(const SectorAround& sectorAround);
	void Sector_UserErase(ChatUser* user, const SectorPos& Pos);
	void Sector_UserInsert(ChatUser* user, const SectorPos& Pos);
	void Sector_SendAround(ChatUser* user, CPacket& packet, bool sendMe);
	void Sector_SendOne(ChatUser* user, CPacket& packet);

	// 섹터의 이동이 될 때마다 주변 섹터에 생성, 삭제를 하는 섹터 이동처리 (MMOTCPFight 같은 섹터 처리)
	void Sector_GetMoveSector(const SectorPos& currentPos, const SectorPos& updatePos, SectorAround& addSectors, SectorAround& removeSectors, SectorAround& seqSector);
	void Sector_SetMoveSector(WORD Case, const SectorPos currentPos, const SectorPos updatePos, SectorAround& addSectors, SectorAround& removeSectors, SectorAround& seqSector);


	// Chat_Packet.cpp
	void Packet_Req_Login(DWORD64 SessionID, CPacket& packet);			// 채팅서버 로그인 요청
	void Packet_Req_Sector_Move(DWORD64 SessionID, CPacket& packet);	// 채팅서버 섹터 이동 요청
	void Packet_Req_Message(DWORD64 SessionID, CPacket& packet);		// 채팅서버 채팅보내기 요청
	void Packet_Req_HeartBeat(DWORD64 SessionID, CPacket& packet);		// 채팅서버 클라이언트 하트비트

	// Chat_InitPacket.cpp
	void InitPacket_ResLogin(CPacket& packet, BYTE Status, INT64 AccountNo);
	void InitPacket_ResSectorMove(CPacket& packet, INT64 AccountNo, WORD SectorX, WORD SectorY);
	void InitPacket_ResMessage(CPacket& packet, INT64 AccountNo, const WCHAR* ID, const WCHAR* Nickname, WORD MessageLen, const WCHAR* Message);


public:
	void OutputDebugStringF(LPCTSTR format, ...);
	ChattingServer2();

	// 모니터링 함수
	void monitor();

	// 섹터별 인원 수 출력
	void SectorUserTextOut();
	void Proc_SectorUserTextOut();

	void GetPCD(PerformanceContentsData& pcd);
};

#endif