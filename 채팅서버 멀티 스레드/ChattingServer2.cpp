#include "ChattingServer2.h"


ChattingServer2::ChattingServer2() : UserPool(100, 200)
{
	// config.ini
	char* pBuffer;
	int size = GetFileSize("ChatServer.ini");
	pBuffer = (char*)malloc(size);
	GetLoadData(pBuffer, size, "ChatServer.ini");

	char Ip[16] = { 0, };
	GetValueChar(pBuffer, Ip, "IP", size);
	size_t ret;
	mbstowcs_s(&ret, IP, 16, Ip, 16);

	GetValueInt(pBuffer, (int*)&LogLevel, "LogLevel", size);
	LOG_LEVEL(LogLevel);

	GetValueInt(pBuffer, (int*)&Port, "Port", size);
	GetValueInt(pBuffer, (int*)&WorkerThread, "WorkerThread", size);
	GetValueInt(pBuffer, (int*)&ActiveThread, "ActiveThread", size);
	GetValueInt(pBuffer, (int*)&Nagle, "Nagle", size);
	GetValueInt(pBuffer, (int*)&SessionMax, "SessionMax", size);
	GetValueInt(pBuffer, (int*)&UserMax, "UserMax", size);
	GetValueInt(pBuffer, (int*)&PacketCode, "PacketCode", size);
	GetValueInt(pBuffer, (int*)&PacketKey, "PacketKey", size);
	GetValueInt(pBuffer, (int*)&UserHeartBeat, "UserHeartBeat", size);
	GetValueInt(pBuffer, (int*)&GuestHeartBeat, "GuestHeartBeat", size);
	free(pBuffer);

	InitializeCriticalSection(&CS_UserMap);
	InitializeCriticalSection(&CS_LoginUserAccountSet);
}
void ChattingServer2::OutputDebugStringF(LPCTSTR format, ...)
{
	WCHAR buffer[1000] = { 0 };

	va_list arg; // 가변인수 벡터

	va_start(arg, format);
	vswprintf_s(buffer, 1000, format, arg);
	va_end(arg);

	OutputDebugString(buffer);
	return;
}


bool ChattingServer2::OnAcceptRequest(WCHAR* IP, WORD Port)
{
	return true;
}
void ChattingServer2::OnSessionConnected(DWORD64 SessionID)
{
	ChatUser* user = UserPool.Alloc();
	user->SessionID = SessionID;


	// 유저 맵 삽입
	EnterCriticalSection(&CS_UserMap);
	UserMap.insert(make_pair(SessionID, user));
	LeaveCriticalSection(&CS_UserMap);

	return;
}
void ChattingServer2::OnSessionRelease(DWORD64 SessionID)
{
	ChatUser* user = UserFind(SessionID);

	SectorPos Pos = { user->SectorX, user->SectorY };
	INT64 AccountID = user->AccountNo;

	Sector_UserErase(user, Pos);

	// 로그인 유저 Set 삭제
	EnterCriticalSection(&CS_LoginUserAccountSet);
	LoginUserAccountSet.erase(AccountID);
	LeaveCriticalSection(&CS_LoginUserAccountSet);

	// 유저 맵 삭제
	EnterCriticalSection(&CS_UserMap);
	UserMap.erase(SessionID);
	LeaveCriticalSection(&CS_UserMap);

	// 초기화
	user->SessionID = INVALID_SESSIONID;
	user->AccountNo = INVALID_ACCOUNTNO;
	user->LastTimeTick = 0;
	user->SectorX = INVALID_SECTOR;
	user->SectorY = INVALID_SECTOR;
	ZeroMemory(&user->ID, en_LEN_ID * 2);
	ZeroMemory(&user->Nickname, en_LEN_NICKNAME * 2);
	ZeroMemory(&user->SessionKey, en_LEN_SESSIONKEY);

	UserPool.Free(user);
	return;
}
void ChattingServer2::OnRecv(DWORD64 SessionID, CPacket& packet)
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

	// 모니터용
	InterlockedIncrement(&Pcd.UpdateTPS);

	return;
}

ChatUser* ChattingServer2::UserFind(DWORD64 SessionID)
{
	ChatUser* pUser = nullptr;
	EnterCriticalSection(&CS_UserMap);
	auto iter = UserMap.find(SessionID);
	if (iter != UserMap.end())
		pUser = iter->second;
	LeaveCriticalSection(&CS_UserMap);
	return pUser;
}

bool ChattingServer2::CompareAccountNo(ChatUser* user, INT64 AccountNo)
{
	if (user->AccountNo != AccountNo)
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"AccountNo Error");
		return false;
	}
	return true;
}
bool ChattingServer2::isDuplicateLogin(INT64 AccountNo)
{
	bool ret;

	EnterCriticalSection(&CS_LoginUserAccountSet);
	auto iter = LoginUserAccountSet.find(AccountNo);
	if (iter == LoginUserAccountSet.end())
		ret = false;
	else
		ret = true;
	LeaveCriticalSection(&CS_LoginUserAccountSet);

	return ret;
}
bool ChattingServer2::isValidToken(const char* sessionKey)
{
	return true;
}
void ChattingServer2::Crash()
{
	// 이 코드는 null 포인터를 역참조하여 크래시를 발생시킵니다.
	int* Nint = NULL;
	*Nint = 0;
}
void ChattingServer2::monitor()
{
	static CCpuUsage CpuUsage;
	PerformanceNetworkData Pnd;
	PerformanceContentsData Pcd;
	GetPND(Pnd);
	GetPCD(Pcd);

	cs_ClearScreen();
	cs_MoveCursor(0, 0);

	wprintf(L"----------------------------------------------\n");
	wprintf(L"SessionCount : %d\n", Pnd.SessionCount);
	wprintf(L"AcceptTotalCount : %d\n", Pnd.AcceptTotalCount);
	wprintf(L"PacketTotalCount: %d\n", Pnd.PacketTotalCount);
	wprintf(L"PacketUseCount: %d\n\n", Pnd.PacketUseCount);

	wprintf(L"AcceptTPS : %d\n", Pnd.AcceptTPS);
	wprintf(L"SendTPS : %d\n", Pnd.SendTPS);
	wprintf(L"RecvTPS : %d\n", Pnd.RecvTPS);
	wprintf(L"----------------------------------------------\n");
	wprintf(L"UserCount : %d\n\n", Pcd.UserCount);

	wprintf(L"UpdateTPS : %d\n", Pcd.UpdateTPS);
	wprintf(L"Message_PacketTPS : %d\n", Pcd.Message_PacketTPS);
	wprintf(L"Message_AcceptTPS : %d\n", Pcd.Message_AcceptTPS);
	wprintf(L"Message_DisconnectTPS : %d\n\n", Pcd.Message_DisconnectTPS);

	wprintf(L"Packet_LoginTPS : %d\n", Pcd.Packet_LoginTPS);
	wprintf(L"Packet_SectorMoveTPS : %d\n", Pcd.Packet_SectorMoveTPS);
	wprintf(L"Packet_MessageTPS : %d\n", Pcd.Packet_MessageTPS);
	//wprintf(L"Packet_HeartBeatTPS : %d\n", Pcd.Packet_HeartBeatTPS);
	SectorUserTextOut();
	wprintf(L"----------------------------------------------\n");
	CpuUsage.UpdateCpuTime();
	wprintf(L"CPU:%.1f%% [Chatting T:%.1f%% K:%.1f%% U:%.1f%%]\n", CpuUsage.ProcessorTotal(), CpuUsage.ProcessTotal(), CpuUsage.ProcessKernel(), CpuUsage.ProcessUser());
	Pdh.monitor();
	wprintf(L"----------------------------------------------");
	return;
}
void ChattingServer2::GetPCD(PerformanceContentsData& pcd)
{
	pcd.UserCount = UserMap.size();
	pcd.UpdateTPS = InterlockedExchange(&Pcd.UpdateTPS, 0);
	pcd.Message_PacketTPS = InterlockedExchange(&Pcd.Message_PacketTPS, 0);
	pcd.Message_AcceptTPS = InterlockedExchange(&Pcd.Message_AcceptTPS, 0);
	pcd.Message_DisconnectTPS = InterlockedExchange(&Pcd.Message_DisconnectTPS, 0);
	pcd.Packet_LoginTPS = InterlockedExchange(&Pcd.Packet_LoginTPS, 0);
	pcd.Packet_SectorMoveTPS = InterlockedExchange(&Pcd.Packet_SectorMoveTPS, 0);
	pcd.Packet_MessageTPS = InterlockedExchange(&Pcd.Packet_MessageTPS, 0);
	pcd.Packet_HeartBeatTPS = InterlockedExchange(&Pcd.Packet_HeartBeatTPS, 0);
	return;
}
void ChattingServer2::Proc_SectorUserTextOut()
{
	FILE* pFile;
	while (_wfopen_s(&pFile, L"SectorUserCount.txt", L"w, ccs=UNICODE") != 0) {}

	// 단순 섹터 인원 출력
	int TotalUserCount = 0;
	for (int Y = 0; Y < en_SECTOR_MAXIMUM_Y; ++Y)
	{
		fwprintf_s(pFile, L"|");
		for (int X = 0; X < en_SECTOR_MAXIMUM_X; ++X)
		{
			TotalUserCount += sectors.sector[Y][X].size();
			fwprintf_s(pFile, L"%3d|", sectors.sector[Y][X].size());
		}
		fwprintf_s(pFile, L"\n");
	}
	fwprintf_s(pFile, L"TotalUserCount		: %d\n", TotalUserCount);
	fwprintf_s(pFile, L"SectorAvrUserCount	: %f", (float)TotalUserCount / (float)(en_SECTOR_MAXIMUM_X * en_SECTOR_MAXIMUM_Y));
	fwprintf_s(pFile, L"\n\n");


	// 주변 Sector 인원 합산
	int Sum = 0;
	for (int Y = 0; Y < en_SECTOR_MAXIMUM_Y; ++Y)
	{
		fwprintf_s(pFile, L"|");
		for (int X = 0; X < en_SECTOR_MAXIMUM_X; ++X)
		{
			SectorAround sectorAround;
			Sector_GetAround(X, Y, sectorAround);

			int sum = 0;
			for (int iCnt = 0; iCnt < sectorAround.SectorCount; ++iCnt)
			{
				sum += sectorAround.Around[iCnt]->size();
			}

			fwprintf_s(pFile, L"%3d|", sum);
			Sum += sum;
		}
		fwprintf_s(pFile, L"\n");
	}
	fwprintf_s(pFile, L"\n\nTotal : %d\n", Sum);
	fwprintf_s(pFile, L"Avr : %.4f\n", (float)Sum / (float)(en_SECTOR_MAXIMUM_X * en_SECTOR_MAXIMUM_Y));
	fwprintf_s(pFile, L"SendTPS = Avr x MessageTPS + SectorMoveTPS + LoginTPS");

	fclose(pFile);

	return;
}
void ChattingServer2::SectorUserTextOut()
{
	// 주변 Sector 인원 합산
	int Sum = 0;
	for (int Y = 0; Y < en_SECTOR_MAXIMUM_Y; ++Y)
	{
		;
		for (int X = 0; X < en_SECTOR_MAXIMUM_X; ++X)
		{
			SectorAround sectorAround;
			Sector_GetAround(X, Y, sectorAround);

			int sum = 0;
			for (int iCnt = 0; iCnt < sectorAround.SectorCount; ++iCnt)
			{
				sum += sectorAround.Around[iCnt]->size();
			}

			Sum += sum;
		}
	}

	wprintf(L"\nSector Avr : %.4f\n", (float)Sum / (float)(en_SECTOR_MAXIMUM_X * en_SECTOR_MAXIMUM_Y));
	return;
}

void ChattingServer2::GuestHeartbeat(LPVOID p)
{
	return;
}
void ChattingServer2::UserHeartbeat(LPVOID p)
{
	return;
}