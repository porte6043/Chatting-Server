#include "ChattingServer2.h"

void ChattingServer2::Packet_Req_Login(DWORD64 SessionID, CPacket& packet)			// ä�ü��� �α��� ��û
{
	CTlsProfile Profile(L"Packet_Req_Login");
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
	
	ChatUser* LoginUser = UserFind(SessionID);

	packet >> LoginUser->AccountNo;
	packet.GetData(LoginUser->ID, en_LEN_ID * 2);
	packet.GetData(LoginUser->Nickname, en_LEN_NICKNAME * 2);
	packet.GetData(LoginUser->SessionKey, en_LEN_SESSIONKEY);
	LoginUser->LastTimeTick = GetTickCount64();
	LoginUser->SectorX = rand() % en_SECTOR_MAXIMUM_X;
	LoginUser->SectorY = rand() % en_SECTOR_MAXIMUM_Y;


	if (!isValidToken(LoginUser->SessionKey))
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"SessionKey Error");
		Disconnect(SessionID);
		return;
	}

	if (isDuplicateLogin(LoginUser->AccountNo))
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"Duplicate Login");
		Disconnect(SessionID);
		return;
	}

	EnterCriticalSection(&CS_LoginUserAccountSet);
	LoginUserAccountSet.insert(LoginUser->AccountNo);
	LeaveCriticalSection(&CS_LoginUserAccountSet);

	InitPacket_ResLogin(packet, 1, LoginUser->AccountNo);
	SendMSG(LoginUser->SessionID, packet);

	return;
}
void ChattingServer2::Packet_Req_Sector_Move(DWORD64 SessionID, CPacket& packet)			// ä�ü��� ���� �̵� ��û
{
	CTlsProfile Profile(L"Packet_Req_Sector_Move");
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

	INT64 AccountNo;
	SectorPos Pos;
	packet >> AccountNo >> Pos.x >> Pos.y;

	ChatUser* User = UserFind(SessionID);

	if (!CompareAccountNo(User, AccountNo)) 
	{
		Disconnect(SessionID);
		return;
	}

	User->LastTimeTick = GetTickCount64();

	if (!Sector_Update(User, Pos))
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"UpdateSector Failed");
		Disconnect(SessionID);
		return;
	}

	InitPacket_ResSectorMove(packet, User->AccountNo, User->SectorX, User->SectorY);
	SendMSG(User->SessionID, packet);

	return;
}
void ChattingServer2::Packet_Req_Message(DWORD64 SessionID, CPacket& packet)				// ä�ü��� ä�ú����� ��û
{
	CTlsProfile Prifile(L"Packet_Req_Message");
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

	INT64 AccountNo;
	WORD MessageLen;
	WCHAR Message[en_LEN_CHATTING];
	packet >> AccountNo >> MessageLen;


	// ���ѵ� Chatting ũ�⸦ �Ѿ� ���� ���
	if (MessageLen > (en_LEN_CHATTING - 1) * 2)
	{
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"Chat Message Buffer Lack");
		Disconnect(SessionID);
		return;
	}

	// ����ִ� ä���� ���� ���
	if (MessageLen == 0)
	{
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"Chat Message Error : MessageLen = 0");
		Disconnect(SessionID);
		return;
	}

	// Message�� ũ�Ⱑ �� �´� ���
	if (packet.GetUseSize() != MessageLen)
	{
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"Chat Message Error : Wrong MessageLen");
		Disconnect(SessionID);
		return;
	}

	packet.GetData(Message, MessageLen);


	ChatUser* user = UserFind(SessionID);

	if (!CompareAccountNo(user, AccountNo)) 
	{
		Disconnect(SessionID);
		return;
	}

	user->LastTimeTick = GetTickCount64();
	InitPacket_ResMessage(packet, AccountNo, user->ID, user->Nickname, MessageLen, Message);
	Sector_SendAround(user, packet, true);

	return;
}
void ChattingServer2::Packet_Req_HeartBeat(DWORD64 SessionID, CPacket& packet)
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

	ChatUser* user = UserFind(SessionID);

	DWORD64 CurrentTimeTick = GetTickCount64();
	if (CurrentTimeTick >= user->LastTimeTick + en_HEARTBEAT_USER)
		Disconnect(SessionID);

	user->LastTimeTick = CurrentTimeTick;

	return;
}