#include "ChattingServer.h"

void ChattingServer::Packet_Req_Login(DWORD64 SessionID, CPacket& packet)			// 채팅서버 로그인 요청
{
	//------------------------------------------------------------
	// 채팅서버 로그인 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null 포함
	//		WCHAR	Nickname[20]		// null 포함
	//		char	SessionKey[64];		// 인증토큰
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

	// 인증토큰 확인 성공
	Guest->LastTimeTick = GetTickCount64();

	UserMap.insert(make_pair(SessionID, Guest));
	AccountSet.insert(Guest->AccountNo);

	InitPacket_ResLogin(packet, 1, Guest->AccountNo);
	SendMSG_PQCS(Guest->SessionID, packet);

	return;
}
void ChattingServer::Packet_Req_Sector_Move(DWORD64 SessionID, CPacket& packet)			// 채팅서버 섹터 이동 요청
{
	//------------------------------------------------------------
	// 채팅서버 섹터 이동 결과
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
void ChattingServer::Packet_Req_Message(DWORD64 SessionID, CPacket& packet)				// 채팅서버 채팅보내기 요청
{
	//------------------------------------------------------------
	// 채팅서버 채팅보내기 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	MessageLen					// Byte 크기
	//		WCHAR	Message[MessageLen / 2]		// null 미포함
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

	// 제한된 Chatting 크기를 넘어 갔을 경우
	if (MessageLen > (en_LEN_CHATTING - 1) * 2)
	{
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"Chat Message Buffer Lack");
		Disconnect(user->SessionID);
		return;
	}

	// 비어있는 채팅을 보낸 경우
	if (MessageLen == 0)
	{
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"Chat Message Error : MessageLen = 0");
		Disconnect(user->SessionID);
		return;
	}

	// Message의 크기가 안 맞는 경우
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
	// 하트비트
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// 클라이언트는 이를 30초마다 보내줌.
	// 서버는 40초 이상동안 메시지 수신이 없는 클라이언트를 강제로 끊어줘야 함.
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