#include "ChattingServer.h"


ChattingServer::ChattingServer()
{
	CTextPasing config;
	config.GetLoadData("ChatServer.ini");

	// ä�� ���� ����
	CTextPasingCategory* ChatServer = config.FindCategory("ChattingServer");
	ChatServer->GetValueWChar(IP, "IP");
	ChatServer->GetValueShort((short*)&Port, "Port");
	ChatServer->GetValueShort((short*)&WorkerThread, "WorkerThreads");
	ChatServer->GetValueShort((short*)&ActiveThread, "ActiveThreads");
	ChatServer->GetValueBool(&Nagle, "Nagle");
	ChatServer->GetValueBool(&ZeroCopySend, "ZeroCopySend");
	ChatServer->GetValueShort((short*)&SessionMax, "SessionMax");
	ChatServer->GetValueByte((char*)&PacketCode, "PacketCode");
	ChatServer->GetValueByte((char*)&PacketKey, "PacketKey");

	ChatServer->GetValueShort((short*)&UserMax, "UserMax");
	ChatServer->GetValueInt((int*)&UserHeartBeat, "UserHeartbeat");
	ChatServer->GetValueInt((int*)&GuestHeartBeat, "GuestHeartbeat");
	ChatServer->GetValueBool(&Heartbeat, "Heartbeat");

	ChatServer->GetValueByte((char*)&LogLevel, "LogLevel");

	MessageQ = &MessageQueue;
	WorkingQ = &WorkingQueue;

	InitializeCriticalSection(&CS_MessageQ);
	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	// Auto Reset

	// ���� ����
	LOG_LEVEL(LogLevel);
	UserPool.TlsPoolInit(1000, UserMax / 1000);

	// �ܼ� �ʱ�ȭ
	cs_Initial();

	// Ÿ�̸� ���
	if(Heartbeat)
	{
		RegisterTimer(UserHeartbeat, this, UserHeartBeat);
		RegisterTimer(GuestHeartbeat, this, GuestHeartBeat);
	}

	// ����� ���� connect
	MonitorClient.Connect(
		MonitorClient.IP,
		MonitorClient.Port,
		MonitorClient.WorkerThreads,
		MonitorClient.ActiveThreads,
		MonitorClient.Nagle,
		MonitorClient.ZeroCopySend,
		MonitorClient.AutoConnect
	);

	// Contents ������ ����
	hContents_Thread = (HANDLE)_beginthreadex(nullptr, 0, ContentsThreadWrapping, this, 0, nullptr);
	if (hContents_Thread == 0)
	{
		int Err = errno;
		LOG(L"Network", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
	}
}

bool ChattingServer::OnAcceptRequest(WCHAR* IP, WORD Port)
{
	return true;
}
void ChattingServer::OnSessionConnected(DWORD64 SessionID)
{
	Message message;
	message.Type = en_MESSAGE_ACCEPT;
	message.SessionID = SessionID;
	MessagePush(message);

	return;
}
void ChattingServer::OnSessionRelease(DWORD64 SessionID)
{
	Message message;
	message.Type = en_MESSAGE_DISCONNECT;
	message.SessionID = SessionID;
	MessagePush(message);
	
	return;
}
void ChattingServer::OnRecv(DWORD64 SessionID, CPacket& packet)
{
	Message message(en_MESSAGE_PACKET, SessionID, packet);
	MessagePush(message);

	return;
}
ChatUser* ChattingServer::FindUser(DWORD64 SessionID)
{
	auto iter = UserMap.find(SessionID);
	if (iter != UserMap.end())
		return iter->second;
	else
		return nullptr;
}
ChatUser* ChattingServer::FindGuest(DWORD64 SessionID)
{
	auto iter = GuestMap.find(SessionID);
	if (iter != GuestMap.end())
		return iter->second;
	else
		return nullptr;
}
void ChattingServer::MessagePush(Message& message)
{
	EnterCriticalSection(&CS_MessageQ);
	MessageQ->push(message);
	LeaveCriticalSection(&CS_MessageQ);
	SetEvent(hEvent);
	return;
}
bool ChattingServer::CompareAccountNo(ChatUser* user, INT64 AccountNo)
{
	if (user->AccountNo != AccountNo)
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"AccountNo Error");
		return false;
	}
	return true;
}
bool ChattingServer::isDuplicateLogin(INT64 AccountNo)
{
	bool Return;
	auto iter = AccountSet.find(AccountNo);
	if (iter == AccountSet.end())
	{
		Return = false;
	}
	else
	{
		ChatUser* DuplicateUser = nullptr;
		for (const auto& iter : UserMap)
		{
			ChatUser* User = iter.second;
			if (User->AccountNo == AccountNo)
			{
				DuplicateUser = User;
				break;
			}
		}

		if (DuplicateUser == nullptr)
			Crash();

		AccountSet_Flag.insert(DuplicateUser->SessionID);
		++Pcd.DuplicateLoginTotalCount;
		
		Disconnect(DuplicateUser->SessionID);
		Return = true;
	}

	return Return;
}
bool ChattingServer::isValidToken(const char* sessionKey)
{
	return true;
}
void ChattingServer::monitor()
{
	PerformanceHardwareData Phd;
	PerformanceNetworkData Pnd;
	PerformanceContentsData Pcd;

	CpuUsage.UpdateCpuTime();
	GetPHD(Phd);
	GetPND(Pnd);
	GetPCD(Pcd);

	// Monitoring ������ ������ ����
	SendToMonitoringServer(Phd, Pnd, Pcd);

	cs_ClearScreen();
	cs_MoveCursor(0, 0);

	wprintf(L"----------------------------------------------\n");
	wprintf(L"MonitoringClient : %s\n", MonitorClient.GetLoginStatus() ? L"ON" : L"OFF");
	wprintf(L"SessionCount : %d\n", Pnd.SessionCount);
	wprintf(L"AcceptTotalCount : %d\n", Pnd.AcceptTotalCount);
	wprintf(L"PacketTotalCount: %d\n", Pnd.PacketTotalCount);
	wprintf(L"PacketUseCount: %d\n\n", Pnd.PacketUseCount);

	wprintf(L"AcceptTPS : %d\n", Pnd.AcceptTPS);
	wprintf(L"SendTPS : %d\n", Pnd.SendTPS);
	wprintf(L"RecvTPS : %d\n", Pnd.RecvTPS);
	wprintf(L"----------------------------------------------\n");
	wprintf(L"UpdateMessage_Queue : %d\n", Pcd.UpdateMessage_Queue);
	wprintf(L"User_Pool : %d\n", Pcd.User_Pool);
	wprintf(L"UserCount : %d\n\n", Pcd.UserCount);

	wprintf(L"UpdateTPS : %d\n", Pcd.UpdateTPS);
	wprintf(L"Message_PacketTPS : %d\n", Pcd.Message_PacketTPS);
	wprintf(L"Message_AcceptTPS : %d\n", Pcd.Message_AcceptTPS);
	wprintf(L"Message_DisconnectTPS : %d\n\n", Pcd.Message_DisconnectTPS);

	wprintf(L"Packet_LoginTPS : %d\n", Pcd.Packet_LoginTPS);
	wprintf(L"Packet_SectorMoveTPS : %d\n", Pcd.Packet_SectorMoveTPS);
	wprintf(L"Packet_MessageTPS : %d\n", Pcd.Packet_MessageTPS);
	SectorUserTextOut();	// Sector ��� �� ���
	wprintf(L"DuplicateLoginTotalCount : %d\n", Pcd.DuplicateLoginTotalCount);
	wprintf(L"----------------------------------------------\n");
	
	wprintf(L"CPU:%.1f%% [Chatting T:%.1f%% K:%.1f%% U:%.1f%%]\n", CpuUsage.ProcessorTotal(), CpuUsage.ProcessTotal(), CpuUsage.ProcessKernel(), CpuUsage.ProcessUser());
	wprintf(L"NonPagePool : %dKByte\n", (unsigned long)(Phd.ProcessNonPagePool / 1024));
	wprintf(L"UsingMemory : %dMByte\n", (unsigned long)(Phd.ProcessUsingMemory / 1024 / 1024));
	wprintf(L"----------------------------------------------");
	return;
}
void ChattingServer::SendToMonitoringServer(PerformanceHardwareData& Phd, PerformanceNetworkData& Pnd, PerformanceContentsData& Pcd)
{
	int Time = (int)time(NULL);

	MonitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, true, Time);
	MonitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, CpuUsage.ProcessTotal(), Time);
	MonitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, Phd.ProcessUsingMemory / 1024 / 1024, Time);
	MonitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_SESSION, Pnd.SessionCount, Time);
	MonitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_PLAYER, Pcd.UserCount, Time);
	MonitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, Pcd.UpdateTPS, Time);
	MonitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, Pnd.PacketUseCount, Time);
	MonitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, Pcd.UpdateMessage_Queue, Time);

	return;
}

void ChattingServer::GetPCD(PerformanceContentsData& pcd)
{
	pcd.UpdateMessage_Queue = MessageQ->size();
	pcd.User_Pool = UserPool.GetTotalCount();
	pcd.UserCount = UserMap.size();
	pcd.UpdateTPS = InterlockedExchange(&Pcd.UpdateTPS, 0);
	pcd.Message_PacketTPS = InterlockedExchange(&Pcd.Message_PacketTPS, 0);
	pcd.Message_AcceptTPS = InterlockedExchange(&Pcd.Message_AcceptTPS, 0);
	pcd.Message_DisconnectTPS = InterlockedExchange(&Pcd.Message_DisconnectTPS, 0);
	pcd.Packet_LoginTPS = InterlockedExchange(&Pcd.Packet_LoginTPS, 0);
	pcd.Packet_SectorMoveTPS = InterlockedExchange(&Pcd.Packet_SectorMoveTPS, 0);
	pcd.Packet_MessageTPS = InterlockedExchange(&Pcd.Packet_MessageTPS, 0);
	pcd.Packet_HeartBeatTPS = InterlockedExchange(&Pcd.Packet_HeartBeatTPS, 0);
	pcd.DuplicateLoginTotalCount = Pcd.DuplicateLoginTotalCount;
	return;
}
void ChattingServer::Proc_SectorUserTextOut()
{
	FILE* pFile;
	while (_wfopen_s(&pFile, L"SectorUserCount.txt", L"w, ccs=UNICODE") != 0) {}

	// �ܼ� ���� �ο� ���
	int TotalUserCount = 0;
	for (int Y = 0; Y < en_SECTOR_MAXIMUM_Y; ++Y)
	{
		fwprintf_s(pFile, L"|");
		for (int X = 0; X < en_SECTOR_MAXIMUM_X; ++X)
		{
			TotalUserCount += Sectors[Y][X].size();
			fwprintf_s(pFile, L"%3d|", Sectors[Y][X].size());
		}
		fwprintf_s(pFile, L"\n");
	}
	fwprintf_s(pFile, L"TotalUserCount		: %d\n", TotalUserCount);
	fwprintf_s(pFile, L"SectorAvrUserCount	: %f", (float)TotalUserCount / (float)(en_SECTOR_MAXIMUM_X * en_SECTOR_MAXIMUM_Y));
	fwprintf_s(pFile, L"\n\n");


	// �ֺ� Sector �ο� �ջ�
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
void ChattingServer::SectorUserTextOut()
{
	// �ֺ� Sector �ο� �ջ�
	int Sum = 0;
	for (int Y = 0; Y < en_SECTOR_MAXIMUM_Y; ++Y)
	{;
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

void ChattingServer::GuestHeartbeat(LPVOID p)
{
	ChattingServer* This = (ChattingServer *)p;

	Message message;
	message.Type = en_MESSAGE_GUESTHEARTBEAT;
	This->MessagePush(message);

	return;
}
void ChattingServer::UserHeartbeat(LPVOID p)
{
	ChattingServer* This = (ChattingServer*)p;

	Message message;
	message.Type = en_MESSAGE_USERHEARTBEAT;
	This->MessagePush(message);

	return;
}

unsigned WINAPI ChattingServer::ContentsThreadWrapping(LPVOID p)
{
	ChattingServer* This = (ChattingServer*)p;
	This->ContentsThread();

	return 0;
}
void ChattingServer::ContentsThread()
{
	while (1)
	{
		WaitForSingleObject(hEvent, INFINITE);

		// Queue ����
		EnterCriticalSection(&CS_MessageQ);
		queue<Message>* temp = MessageQ;
		MessageQ = WorkingQ;
		WorkingQ = temp;
		LeaveCriticalSection(&CS_MessageQ);

		// ��¥ ���� ó��
		if (WorkingQ->size() == 0)
			continue;

		while (1)
		{
			if (WorkingQ->size() == 0)
				break;

			Message message = WorkingQ->front();
			WorkingQ->pop();


			// contents ó��
			switch (message.Type)
			{
			case en_MESSAGE_PACKET:
				Proc_Packet(message.SessionID, message.Packet);
				InterlockedIncrement(&Pcd.Message_PacketTPS);
				break;

			case en_MESSAGE_ACCEPT:
				Proc_Accept(message.SessionID, message.Packet);
				InterlockedIncrement(&Pcd.Message_AcceptTPS);
				break;

			case en_MESSAGE_DISCONNECT:
				Proc_Disconnect(message.SessionID, message.Packet);
				InterlockedIncrement(&Pcd.Message_DisconnectTPS);
				break;

			case en_MESSAGE_USERHEARTBEAT:
				Proc_UserHeartbeat();
				break;

			case en_MESSAGE_GUESTHEARTBEAT:
				Proc_GuestHeartbeat();
				break;
			default:
				break;
			}

			// ����Ϳ�
			InterlockedIncrement(&Pcd.UpdateTPS);
		}
	}

	return;
}