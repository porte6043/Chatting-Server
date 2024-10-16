#ifndef  __CHATTINGSERVER__
#define __CHATTINGSERVER__

#include <unordered_set>
#include "��Ʈ��ũ/CWanServer.h"
#include "st_ChatServer.h"
#include "ChattingProtocol.h"

#include "MonitoringClient_ChattingServer.h"


#define INVALID_ACCOUNTNO (INT64)-1

class ChattingServer : public CWanServer
{
public:
	struct PerformanceContentsData
	{
		DWORD UpdateMessage_Queue;		// UpdateThread ť ���� ���� (MessageQ.size)
		DWORD User_Pool;				// User ����ü �Ҵ緮 (UserPool.Size = GuestMap.Size + UserMap.Size)
		DWORD UserCount;				// �α��� �� User ���� (UserMap.UseSize)
		DWORD UpdateTPS;				// �ʴ� Update ó�� Ƚ�� (Message_Packet + Message_Accept + Message_Disconnect)
		DWORD Message_PacketTPS;		// Message_Packet (Message_Packet)
		DWORD Message_AcceptTPS;		// Message_Accept (Message_Accept)
		DWORD Message_DisconnectTPS;	// Message_Disconnect (Message_Disconnect)
		DWORD Packet_LoginTPS;			// Packet_Req_Login
		DWORD Packet_SectorMoveTPS;		// Packet_Req_SectorMove
		DWORD Packet_MessageTPS;		// Packet_Req_Message
		DWORD Packet_HeartBeatTPS;		// Packet_Req_HeartBeat
		DWORD DuplicateLoginTotalCount; // �ߺ� �α��� �� ī��Ʈ

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
	HANDLE hContents_Thread;										// Contents ������

	queue<Message> MessageQueue;
	queue<Message> WorkingQueue;
	queue<Message>* MessageQ;
	queue<Message>* WorkingQ;
	CRITICAL_SECTION CS_MessageQ;
	HANDLE hEvent;

	CTlsPool<ChatUser> UserPool;									// ���� Ǯ
	unordered_set<INT64> AccountSet;								// �α����� ������ �ߺ� Ȯ�� üũ // ���߿� Map���� ����
	unordered_set<DWORD64> AccountSet_Flag;
	unordered_map<DWORD64, ChatUser*> UserMap;						// �α��� �� ������
	unordered_map<DWORD64, ChatUser*> GuestMap;						// �α��� �� ������
	Sector Sectors[en_SECTOR_MAXIMUM_Y][en_SECTOR_MAXIMUM_X];		// ����

	CCpuUsage CpuUsage;												// �ϵ���� ������
	PerformanceContentsData Pcd;									// ����͸� ������

	CMonitoringClient_Chatting MonitorClient;						// ����͸� Ŭ���̾�Ʈ

public:
	// ���� ����
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

	// ȭ��Ʈ IP�� Port ���� ���θ� �Ǵ��մϴ�. (Accpet ��)
	virtual bool OnAcceptRequest(WCHAR* IP, WORD Port);

	// Accept �� Session�� ������ �� �������� Client�� Session ������ �˸��ϴ�.
	virtual void OnSessionConnected(DWORD64 SessionID);

	// Session�� ���� �� �� ȣ���մϴ�.
	virtual void OnSessionRelease(DWORD64 SessionID);

	// Message ���� �Ϸ� �� Contents Packet ó���� �մϴ�.
	virtual void OnRecv(DWORD64 SessionID, CPacket& packet);

	ChatUser* FindUser(DWORD64 SessionID);

	ChatUser* FindGuest(DWORD64 SessionID);

	void MessagePush(Message& message);

	bool isDuplicateLogin(INT64 AccountNo);

	bool isValidToken(const char* sessionKey);

	// ��ȯ�� �ٸ��� false ������ true
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
	void Packet_Req_Login(DWORD64 SessionID, CPacket& packet);			// ä�ü��� �α��� ��û
	void Packet_Req_Sector_Move(DWORD64 SessionID, CPacket& packet);	// ä�ü��� ���� �̵� ��û
	void Packet_Req_Message(DWORD64 SessionID, CPacket& packet);		// ä�ü��� ä�ú����� ��û
	void Packet_Req_HeartBeat(DWORD64 SessionID, CPacket& packet);		// ä�ü��� Ŭ���̾�Ʈ ��Ʈ��Ʈ

	// Chat_CreateMessage.cpp
	void InitPacket_ResLogin(CPacket& packet, BYTE Status, INT64 AccountNo);
	void InitPacket_ResSectorMove(CPacket& packet, INT64 AccountNo, WORD SectorX, WORD SectorY);
	void InitPacket_ResMessage(CPacket& packet, INT64 AccountNo, const WCHAR* ID, const WCHAR* Nickname, WORD MessageLen, const WCHAR* Message);


public:
	ChattingServer();

	// ����͸� �Լ�
	void monitor();

	// ���ͺ� �ο� �� ���
	void SectorUserTextOut();
	void Proc_SectorUserTextOut();

	void GetPCD(PerformanceContentsData& pcd);
};
#endif