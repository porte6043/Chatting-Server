#ifndef __MONITTORCLIENT_CHATTING__
#define __MONITTORCLIENT_CHATTING__

#include "匙飘况农/CLanClient.h"
#include "MonitorProtocol.h"

class CMonitoringClient_Chatting : public CLanClient
{
private:
	bool LoginStatus;

public:
	// System
	wchar_t IP[16];				// Server IP
	WORD Port;					// Server Port
	WORD WorkerThreads;			// WorkerThread狼 荐
	WORD ActiveThreads;			// ActiveThread狼 荐
	bool Nagle;					// Client Nagle 可记
	bool ZeroCopySend;			// ZeroCopySend 可记
	bool AutoConnect;			// 磊悼 connect 可记



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