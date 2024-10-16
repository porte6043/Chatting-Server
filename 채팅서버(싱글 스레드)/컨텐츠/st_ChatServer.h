#ifndef __MESSAGEQUEUE__
#define __MESSAGEQUEUE__
#include <Windows.h>
#include <queue>
#include <list>
#include "���� ���̺귯��/SerialRingBuffer.h"

#define dfPACKET_CODE		0x77
#define dfPACKET_KEY		0x32
#define INVALID_USER		0

enum en_CHATSERVER_CONFIG
{
	en_LEN_ID = 20,
	en_LEN_NICKNAME = 20,
	en_LEN_SESSIONKEY = 64,
	en_LEN_CHATTING = 256,

	en_SECTOR_MAXIMUM_X = 50,
	en_SECTOR_MAXIMUM_Y = 50,

	en_HEARTBEAT_USER = 40000,
	en_HEARTBEAT_GUEST = 40000
};


enum en_MESSAGE_TYPE
{
	en_MESSAGE_PACKET = 0,
	en_MESSAGE_ACCEPT,
	en_MESSAGE_DISCONNECT,
	en_MESSAGE_GUESTHEARTBEAT,
	en_MESSAGE_USERHEARTBEAT
};

struct ChatUser
{
	DWORD64 SessionID;
	INT64 AccountNo;
	WCHAR ID[en_LEN_ID];
	WCHAR Nickname[en_LEN_NICKNAME];
	char SessionKey[en_LEN_SESSIONKEY];
	DWORD64 LastTimeTick;
	WORD SectorX;
	WORD SectorY;

	ChatUser() : ID{}, Nickname{}, SessionKey{}
	{
		SessionID = -1;
		AccountNo = -1;
		LastTimeTick = 0;
		SectorX = -1;
		SectorY = -1;
	};
};

struct Sector : public list<ChatUser*>
{
	WORD SectorX;
	WORD SectorY;
};

struct SectorAround
{
	int SectorCount;
	Sector* Around[9];

	SectorAround() : SectorCount(0), Around{} {}
};


struct Message
{
	WORD Type;
	DWORD64 SessionID;
	CPacket Packet;

public:
	Message()
	{
		Type = 0;
		SessionID = 0;
	}
public:
	Message(WORD type, DWORD64 sessionID, CPacket packet) : Type  (type), SessionID(sessionID), Packet(packet) {}
};

struct QUEUE : public std::queue<Message>
{
	CRITICAL_SECTION CS_Queue;	// Contents ó���� Queue�� ����ȭ ��ü
	HANDLE hEvent;				// Contents �����带 ����� ���� �̺�Ʈ ��ü

	QUEUE()
	{
		InitializeCriticalSection(&CS_Queue);
		hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	// Auto Reset
	}
};
#endif