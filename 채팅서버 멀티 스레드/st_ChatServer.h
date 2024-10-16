#ifndef __MESSAGEQUEUE__
#define __MESSAGEQUEUE__
#include <Windows.h>
#include <queue>
#include <list>
#include "SerialRingBuffer.h"
#include "CNetwork.h"
using namespace std;

#define dfPACKET_CODE		0x77
#define dfPACKET_KEY		0x32
#define INVALID_USER		0
#define INVALID_ACCOUNTNO (INT64)-1
#define INVALID_SECTOR		-1


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
	en_MESSAGE_USERHEARTBEAT,
	en_MESSAGE_SECTOR_DATA_TEXTOUT
};

enum en_STATE_USER
{
	en_STATE_USER = 0,
	en_STATE_GUEST,
	en_STATE_EMPTY
};


struct ChatUser;
struct SectorPos;
struct SectorAround;
struct Sector;
struct Sectors;


struct lock_cs
{
	CRITICAL_SECTION* cs;

	lock_cs(CRITICAL_SECTION* CS)
	{
		cs = CS;
		EnterCriticalSection(cs);
	}
	~lock_cs()
	{
		LeaveCriticalSection(cs);
	}
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
	CRITICAL_SECTION CS_User;

	ChatUser() : ID{}, Nickname{}, SessionKey{}
	{
		SessionID = INVALID_SESSIONID;
		AccountNo = INVALID_ACCOUNTNO;
		LastTimeTick = 0;
		SectorX = INVALID_SECTOR;
		SectorY = INVALID_SECTOR;
		InitializeCriticalSection(&CS_User);
	};
};



struct SectorPos
{
	WORD x;
	WORD y;

	SectorPos()
	{
		x = INVALID_SECTOR;
		y = INVALID_SECTOR;
	}
	SectorPos(WORD X, WORD Y) : x(X), y(Y) {}
};

struct SectorAround
{
	int SectorCount;
	Sector* Around[12];

	SectorAround() : Around{}
	{
		SectorCount = 0;
	}
};

struct Sector : public std::list<ChatUser*>
{
	SectorPos Pos;
	WORD Seq;
	SectorAround Around;
	CRITICAL_SECTION CS_Sector;
	int count;

	Sector()
	{
		Seq = -1;
		count = 0;
		InitializeCriticalSection(&CS_Sector);
	}
};

struct Sectors
{
	Sector sector[en_SECTOR_MAXIMUM_Y][en_SECTOR_MAXIMUM_X];

	Sectors()
	{
		int Seq = 0;
		for (int Y = 0; Y < en_SECTOR_MAXIMUM_Y; ++Y)
		{
			for (int X = 0; X < en_SECTOR_MAXIMUM_X; ++X)
			{
				sector[Y][X].Pos.x = X;
				sector[Y][X].Pos.y = Y;
				sector[Y][X].Seq = Seq++;

				// 주변 섹터 저장
				int index = 0;
				for (int _Y = -1; _Y <= 1; ++_Y) // 열
				{
					for (int _X = -1; _X <= 1; ++_X)	// 행
					{
						int newSectorX = X + _X;
						int newSectorY = Y + _Y;

						// 새로운 인덱스가 범위내에 있는지 확인합니다
						if (newSectorX >= 0 && newSectorX < en_SECTOR_MAXIMUM_X &&
							newSectorY >= 0 && newSectorY < en_SECTOR_MAXIMUM_Y)
						{
							sector[Y][X].Around.Around[index] = &sector[newSectorY][newSectorX];
							++sector[Y][X].Around.SectorCount;
							++index;
						}
					}
				}
			}
		}
	}
};

#endif