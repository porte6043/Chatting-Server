#ifndef __MONITTORCLIENT_CHATTING__
#define __MONITTORCLIENT_CHATTING__

#include "��Ʈ��ũ/CLanClient.h"
#include "MonitorProtocol.h"

class CMonitoringClient_Chatting : public CLanClient
{
private:
	bool LoginStatus;

public:
	// System
	wchar_t IP[16];				// Server IP
	WORD Port;					// Server Port
	WORD WorkerThreads;			// WorkerThread�� ��
	WORD ActiveThreads;			// ActiveThread�� ��
	bool Nagle;					// Client Nagle �ɼ�
	bool ZeroCopySend;			// ZeroCopySend �ɼ�
	bool AutoConnect;			// �ڵ� connect �ɼ�



public:		CMonitoringClient_Chatting();
public:		~CMonitoringClient_Chatting();

private:	virtual void OnConnectServer();
protected:	virtual void OnDisconnectServer();
protected:	virtual void OnRecv(CPacket& packet);

private:	void Packet_Login(CPacket& packet);

public:		void SendMonitorData(BYTE dataType, int dataValue, int timeStamp);
public:		bool GetLoginStatus();

};


#endif