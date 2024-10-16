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
		DWORD UserCount;				// �α��� �� User ���� (UserMap.UseSize)
		DWORD UpdateTPS;				// �ʴ� Update ó�� Ƚ�� (Message_Packet + Message_Accept + Message_Disconnect)
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
	unordered_map<DWORD64, ChatUser*> UserMap;				// ���� �迭�� �˻� ��
	CRITICAL_SECTION CS_UserMap;

	set<INT64> LoginUserAccountSet;
	CRITICAL_SECTION CS_LoginUserAccountSet;

	CTlsPool<ChatUser> UserPool;

	Sectors sectors;
	PerformanceContentsData Pcd;									// ����͸� ������

public:
	// ���� ����
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
	// ȭ��Ʈ IP�� Port ���� ���θ� �Ǵ��մϴ�. (Accpet ��)
	virtual bool OnAcceptRequest(WCHAR* IP, WORD Port);
	// Accept �� Session�� ������ �� �������� Client�� Session ������ �˸��ϴ�.
	virtual void OnSessionConnected(DWORD64 SessionID);
	// Session�� ���� �� �� ȣ���մϴ�.
	virtual void OnSessionRelease(DWORD64 SessionID);
	// Message ���� �Ϸ� �� Contents Packet ó���� �մϴ�.
	virtual void OnRecv(DWORD64 SessionID, CPacket& packet);

	ChatUser* UserFind(DWORD64 SessionID);

	bool isDuplicateLogin(INT64 AccountNo);
	bool isValidToken(const char* sessionKey);

	// ��ȯ�� �ٸ��� false ������ true
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

	// ������ �̵��� �� ������ �ֺ� ���Ϳ� ����, ������ �ϴ� ���� �̵�ó�� (MMOTCPFight ���� ���� ó��)
	void Sector_GetMoveSector(const SectorPos& currentPos, const SectorPos& updatePos, SectorAround& addSectors, SectorAround& removeSectors, SectorAround& seqSector);
	void Sector_SetMoveSector(WORD Case, const SectorPos currentPos, const SectorPos updatePos, SectorAround& addSectors, SectorAround& removeSectors, SectorAround& seqSector);


	// Chat_Packet.cpp
	void Packet_Req_Login(DWORD64 SessionID, CPacket& packet);			// ä�ü��� �α��� ��û
	void Packet_Req_Sector_Move(DWORD64 SessionID, CPacket& packet);	// ä�ü��� ���� �̵� ��û
	void Packet_Req_Message(DWORD64 SessionID, CPacket& packet);		// ä�ü��� ä�ú����� ��û
	void Packet_Req_HeartBeat(DWORD64 SessionID, CPacket& packet);		// ä�ü��� Ŭ���̾�Ʈ ��Ʈ��Ʈ

	// Chat_InitPacket.cpp
	void InitPacket_ResLogin(CPacket& packet, BYTE Status, INT64 AccountNo);
	void InitPacket_ResSectorMove(CPacket& packet, INT64 AccountNo, WORD SectorX, WORD SectorY);
	void InitPacket_ResMessage(CPacket& packet, INT64 AccountNo, const WCHAR* ID, const WCHAR* Nickname, WORD MessageLen, const WCHAR* Message);


public:
	void OutputDebugStringF(LPCTSTR format, ...);
	ChattingServer2();

	// ����͸� �Լ�
	void monitor();

	// ���ͺ� �ο� �� ���
	void SectorUserTextOut();
	void Proc_SectorUserTextOut();

	void GetPCD(PerformanceContentsData& pcd);
};

#endif