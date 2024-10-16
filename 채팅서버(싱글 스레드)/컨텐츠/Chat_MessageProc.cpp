#include "ChattingServer.h"

void ChattingServer::Proc_Accept(DWORD64 SessionID, CPacket& packet) 
{
	ChatUser* Guest = UserPool.Alloc();
	Guest->SessionID = SessionID;
	Guest->AccountNo = -1;
	ZeroMemory(Guest->ID, en_LEN_ID);
	ZeroMemory(Guest->Nickname, en_LEN_NICKNAME);
	ZeroMemory(Guest->SessionKey, en_LEN_SESSIONKEY);
	Guest->LastTimeTick = GetTickCount64();
	Guest->SectorX = -1;
	Guest->SectorY = -1;

	GuestMap.insert(make_pair(SessionID, Guest));

	return;
}
void ChattingServer::Proc_Disconnect(DWORD64 SessionID, CPacket& packet)
{
	ChatUser* DisconnectUser = FindUser(SessionID);
	if (DisconnectUser == nullptr) // 평범한 유저의 종료
	{
		DisconnectUser = FindGuest(SessionID);
		if (DisconnectUser == nullptr)
		{
			LOG(L"ChatServer", CLog::LEVEL_ERROR, L"DisconnectUser Not Found");
			Crash();
		}
		GuestMap.erase(SessionID);
	}
	else // 중복로그인
	{
		Sector_UserErase(DisconnectUser);
		UserMap.erase(SessionID);
		if (AccountSet_Flag.end() != AccountSet_Flag.find(SessionID))
		{
			AccountSet_Flag.erase(SessionID);
		}
		else
		{
			AccountSet.erase(DisconnectUser->AccountNo);
		}
		
	}

	UserPool.Free(DisconnectUser);
	return;
}
void ChattingServer::Proc_Packet(DWORD64 SessionID, CPacket& packet)
{
	WORD Type;
	packet >> Type;
	switch (Type)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		Packet_Req_Login(SessionID, packet);
		InterlockedIncrement(&Pcd.Packet_LoginTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		Packet_Req_Sector_Move(SessionID, packet);
		InterlockedIncrement(&Pcd.Packet_SectorMoveTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		Packet_Req_Message(SessionID, packet);
		InterlockedIncrement(&Pcd.Packet_MessageTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		Packet_Req_HeartBeat(SessionID, packet);
		InterlockedIncrement(&Pcd.Packet_HeartBeatTPS);
		break;

	default:
		LOG(L"ChatServer", CLog::LEVEL_ERROR, L"Packet Error");
		Disconnect(SessionID);
		break;
	}

	return;
}
void ChattingServer::Proc_UserHeartbeat()
{
	DWORD64 CurrentTimeTick = GetTickCount64();

	for (auto iter = UserMap.begin(); iter != UserMap.end(); ++iter)
	{
		ChatUser* user = (*iter).second;
		if (CurrentTimeTick >= user->LastTimeTick + en_HEARTBEAT_USER)
		{
			LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"User Heartbeart Time Out");
			Disconnect(user->SessionID);
		}
	}

	return;
}
void ChattingServer::Proc_GuestHeartbeat()
{
	static int ghCount = 0;
	DWORD64 CurrentTimeTick = GetTickCount64();

	for (auto iter = GuestMap.begin(); iter != GuestMap.end(); ++iter)
	{
		ChatUser* Guest = (*iter).second;
		if (CurrentTimeTick >= Guest->LastTimeTick + en_HEARTBEAT_GUEST)
		{
			ghCount++;
			OutputDebugStringF(L"GHCount = %d\n", ghCount);
			
			LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"Guest Heartbeart Time Out");
			Disconnect(Guest->SessionID);
		}
	}

	return;
}