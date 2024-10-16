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
	// �α��� ��Ŷ ������
	// LoginServer, GameServer , ChatServer  �� ����͸� ������ �α��� ��
	//
	// 
	//	{
	//		WORD	Type
	//
	//		BYTE	ServerNo		//  �� �������� ���� ��ȣ�� �ο��Ͽ� ���
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
		// Len ȯ���̶� Type�� ���� ��Ŷ�� �� ��� ����
		LOG(L"MonitoringClient_ChattingServer", CLog::LEVEL_ERROR, L"unknown packet");
	}

	return;
}

void CMonitoringClient_Chatting::Packet_Login(CPacket& packet)
{
	//------------------------------------------------------------
	// LoginServer, GameServer , ChatServer  �� ����͸� ������ �α��� ��
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
	// ������ ����͸������� ������ ����
	// �� ������ �ڽ��� ����͸����� ��ġ�� 1�ʸ��� ����͸� ������ ����.
	//
	// ������ �ٿ� �� ��Ÿ ������ ����͸� �����Ͱ� ���޵��� ���ҋ��� ����Ͽ� TimeStamp �� �����Ѵ�.
	// �̴� ����͸� Ŭ���̾�Ʈ���� ���,�� ����Ѵ�.
	// 
	//	{
	//		WORD	Type
	//
	//		BYTE	DataType				// ����͸� ������ Type �ϴ� Define ��.
	//		int		DataValue				// �ش� ������ ��ġ.
	//		int		TimeStamp				// �ش� �����͸� ���� �ð� TIMESTAMP  (time() �Լ�)
	//										// ���� time �Լ��� time_t Ÿ�Ժ����̳� 64bit �� ���񽺷����
	//										// int �� ĳ�����Ͽ� ����. �׷��� 2038�� ������ ��밡��
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