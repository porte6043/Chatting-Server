#include "MonitoringClient_ChattingServer.h"


CMonitoringClient_Chatting::CMonitoringClient_Chatting()
{
	CTextPasing config;
	config.GetLoadData("ChatServer.ini");
	CTextPasingCategory* category = config.FindCategory("MonitoringClient_Chatting");
	category->GetValueWChar(IP, "IP");
	category->GetValueShort((short*)&Port, "Port");
	category->GetValueBool(&Nagle, "Nagle");
	category->GetValueBool(&ZeroCopySend, "ZeroCopySend");
	category->GetValueBool(&AutoConnect, "AutoConnect");
	category->GetValueShort((short*)&WorkerThreads, "WorkerThreads");
	category->GetValueShort((short*)&ActiveThreads, "ActiveThreads");

	LoginStatus = false;
}

CMonitoringClient_Chatting::~CMonitoringClient_Chatting()
{

}

void CMonitoringClient_Chatting::OnConnectServer()
{
	//------------------------------------------------------------
	// 로그인 패킷 보내기
	// LoginServer, GameServer , ChatServer  가 모니터링 서버에 로그인 함
	//
	// 
	//	{
	//		WORD	Type
	//
	//		BYTE	ServerNo		//  각 서버마다 고유 번호를 부여하여 사용
	//	}
	//
	//------------------------------------------------------------
	WORD Type = en_PACKET_MONITOR_REQ_LOGIN;
	BYTE ServerNo = dfSERVER_CHATTING;
	
	CPacket packet = CPacket::Alloc();
	packet << Type << ServerNo;
	SendMSG(packet);

	return;
}
void CMonitoringClient_Chatting::OnDisconnectServer()
{
	LoginStatus = false;

	return;
}
void CMonitoringClient_Chatting::OnRecv(CPacket& packet)
{
	WORD Type;
	packet >> Type;
	
	switch (Type)
	{
	case en_PACKET_MONITOR_RES_LOGIN:
		Packet_Login(packet);
		break;
	default:
		// Len 환경이라 Type에 없는 패킷이 올 경우 에러
		LOG(L"MonitoringClient_ChattingServer", CLog::LEVEL_ERROR, L"unknown packet");
	}

	return;
}

void CMonitoringClient_Chatting::Packet_Login(CPacket& packet)
{
	//------------------------------------------------------------
	// LoginServer, GameServer , ChatServer  가 모니터링 서버에 로그인 함
	//
	// 
	//	{
	//		WORD	Type
	//	}
	//
	//------------------------------------------------------------

	LoginStatus = true;
	
	return;
}
bool CMonitoringClient_Chatting::GetLoginStatus()
{
	return LoginStatus;
}

void CMonitoringClient_Chatting::SendMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
	//------------------------------------------------------------
	// 서버가 모니터링서버로 데이터 전송
	// 각 서버는 자신이 모니터링중인 수치를 1초마다 모니터링 서버로 전송.
	//
	// 서버의 다운 및 기타 이유로 모니터링 데이터가 전달되지 못할떄를 대비하여 TimeStamp 를 전달한다.
	// 이는 모니터링 클라이언트에서 계산,비교 사용한다.
	// 
	//	{
	//		WORD	Type
	//
	//		BYTE	DataType				// 모니터링 데이터 Type 하단 Define 됨.
	//		int		DataValue				// 해당 데이터 수치.
	//		int		TimeStamp				// 해당 데이터를 얻은 시간 TIMESTAMP  (time() 함수)
	//										// 본래 time 함수는 time_t 타입변수이나 64bit 로 낭비스러우니
	//										// int 로 캐스팅하여 전송. 그래서 2038년 까지만 사용가능
	//	}
	//
	//------------------------------------------------------------

	if (!LoginStatus)
		return;

	CPacket packet = CPacket::Alloc();
	WORD Type = en_PACKET_CHATTINGSERVER_MONITOR_DATA_UPDATE;
	packet << Type << dataType << dataValue << timeStamp;

	SendMSG_PQCS(packet);

	return;
}