#include "ChattingServer.h"

void ChattingServer::Packet_Req_Login(DWORD64 SessionID, CPacket& packet)			// ä�ü��� �α��� ��û
{
	//------------------------------------------------------------
	// ä�ü��� �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null ����
	//		WCHAR	Nickname[20]		// null ����
	//		char	SessionKey[64];		// ������ū
	//	}
	//
	//------------------------------------------------------------
	if (packet.GetUseSize() != sizeof(ChatUser::AccountNo) + en_LEN_ID * 2 + en_LEN_NICKNAME * 2 + en_LEN_SESSIONKEY)
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"Packet Error : Payload Len Mismatch");
		Disconnect(SessionID);
		return;
	}
	
	ChatUser* Guest = FindGuest(SessionID);
	if (Guest == nullptr)
	{
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"Guest Not Found");
		return;
	}
	GuestMap.erase(SessionID);

	Guest->SessionID = SessionID;
	packet >> Guest->AccountNo;
	packet.GetData(Guest->ID, en_LEN_ID * 2);
	packet.GetData(Guest->Nickname, en_LEN_NICKNAME * 2);
	packet.GetData(Guest->SessionKey, en_LEN_SESSIONKEY);
	Guest->SectorX = rand() % en_SECTOR_MAXIMUM_X;
	Guest->SectorY = rand() % en_SECTOR_MAXIMUM_Y;

	if (!isValidToken(Guest->SessionKey))
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"SessionKey Error");
		Disconnect(Guest->SessionID);
		return;
	}

	if (isDuplicateLogin(Guest->AccountNo))
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"Duplicate Login");
	}

	// ������ū Ȯ�� ����
	Guest->LastTimeTick = GetTickCount64();

	UserMap.insert(make_pair(SessionID, Guest));
	AccountSet.insert(Guest->AccountNo);

	InitPacket_ResLogin(packet, 1, Guest->AccountNo);
	SendMSG_PQCS(Guest->SessionID, packet);

	return;
}
void ChattingServer::Packet_Req_Sector_Move(DWORD64 SessionID, CPacket& packet)			// ä�ü��� ���� �̵� ��û
{
	//------------------------------------------------------------
	// ä�ü��� ���� �̵� ���
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	if (packet.GetUseSize() != sizeof(ChatUser::AccountNo) + sizeof(ChatUser::SectorX) + sizeof(ChatUser::SectorY))
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"Packet Error : Payload Len Mismatch");
		Disconnect(SessionID);
		return;
	}

	ChatUser* user = FindUser(SessionID);
	if (user == nullptr)
	{
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"ChatUser Not Found");
		return;
	}

	INT64 AccountNo;
	WORD SectorX;
	WORD SectorY;
	packet >> AccountNo >> SectorX >> SectorY;
 
	if (!CompareAccountNo(user, AccountNo)) 
	{
		Disconnect(SessionID);
		return;
	}

	user->LastTimeTick = GetTickCount64();

	if (!Sector_Update(user, SectorX, SectorY))
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"UpdateSector Failed");
		Disconnect(SessionID);
		return;
	}

	InitPacket_ResSectorMove(packet, user->AccountNo, user->SectorX, user->SectorY);
	SendMSG_PQCS(user->SessionID, packet);

	return;
}
void ChattingServer::Packet_Req_Message(DWORD64 SessionID, CPacket& packet)				// ä�ü��� ä�ú����� ��û
{
	//------------------------------------------------------------
	// ä�ü��� ä�ú����� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	MessageLen					// Byte ũ��
	//		WCHAR	Message[MessageLen / 2]		// null ������
	//	}
	//
	//------------------------------------------------------------
	if (packet.GetUseSize() > sizeof(ChatUser::AccountNo) + sizeof(WORD) + en_LEN_CHATTING * 2)
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"Packet Error : Payload Len Mismatch");
		Disconnect(SessionID);
		return;
	}

	ChatUser* user = FindUser(SessionID);
	if (user == nullptr) 
	{
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"ChatUser Not Found");
		return;
	}

	INT64 AccountNo;
	WORD MessageLen;
	WCHAR Message[en_LEN_CHATTING];

	packet >> AccountNo >> MessageLen;
	if (!CompareAccountNo(user, AccountNo)) 
	{
		Disconnect(SessionID);
		return;
	}

	// ���ѵ� Chatting ũ�⸦ �Ѿ� ���� ���
	if (MessageLen > (en_LEN_CHATTING - 1) * 2)
	{
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"Chat Message Buffer Lack");
		Disconnect(user->SessionID);
		return;
	}

	// ����ִ� ä���� ���� ���
	if (MessageLen == 0)
	{
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"Chat Message Error : MessageLen = 0");
		Disconnect(user->SessionID);
		return;
	}

	// Message�� ũ�Ⱑ �� �´� ���
	if (packet.GetUseSize() != MessageLen)
	{
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"Chat Message Error : Wrong MessageLen");
		Disconnect(user->SessionID);
		return;
	}

	packet.GetData(Message, MessageLen);

	user->LastTimeTick = GetTickCount64();
	InitPacket_ResMessage(packet, AccountNo, user->ID, user->Nickname, MessageLen, Message);
	Sector_SendAround(user, packet, true);

	return;
}
void ChattingServer::Packet_Req_HeartBeat(DWORD64 SessionID, CPacket& packet)
{
	//------------------------------------------------------------
	// ��Ʈ��Ʈ
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// Ŭ���̾�Ʈ�� �̸� 30�ʸ��� ������.
	// ������ 40�� �̻󵿾� �޽��� ������ ���� Ŭ���̾�Ʈ�� ������ ������� ��.
	//------------------------------------------------------------
	WORD Type;
	packet >> Type;

	ChatUser* user = FindUser(SessionID);
	{
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"ChatUser Not Found");
		if (user == nullptr) return;
	}

	DWORD64 CurrentTimeTick = GetTickCount64();
	if (CurrentTimeTick >= user->LastTimeTick + en_HEARTBEAT_USER)
	{
		Disconnect(SessionID);
	}

	user->LastTimeTick = CurrentTimeTick;

	return;
}