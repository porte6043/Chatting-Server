#include "ChattingServer.h"

void ChattingServer::Sector_UserErase(ChatUser* user)
{
	Sector* pSector = &Sectors[user->SectorY][user->SectorX];
	for (auto iter = pSector->begin(); iter != pSector->end(); ++iter)
	{
		if (*iter == user)
		{
			pSector->erase(iter);
			break;
		}
	}

	//for(auto &i : Sectors[user->SectorY][user->SectorX]) // 향상된 for문

	return;
}
void ChattingServer::Sector_GetAround(ChatUser* user, SectorAround& sectorAround)
{
	int index = 0;
	for (int Y = -1; Y <= 1; ++Y) // 열
	{
		for (int X = -1; X <= 1; ++X)	// 행
		{
			int newSectorX = user->SectorX + X;
			int newSectorY = user->SectorY + Y;

			// 새로운 인덱스가 범위내에 있는지 확인합니다
			if (newSectorX >= 0 && newSectorX < en_SECTOR_MAXIMUM_X &&
				newSectorY >= 0 && newSectorY < en_SECTOR_MAXIMUM_Y)
			{
				sectorAround.Around[index] = &Sectors[newSectorY][newSectorX];
				++sectorAround.SectorCount;
				++index;
			}
		}
	}
	return;
}
void ChattingServer::Sector_GetAround(WORD x, WORD y, SectorAround& sectorAround)
{
	int index = 0;
	for (int Y = -1; Y <= 1; ++Y) // 열
	{
		for (int X = -1; X <= 1; ++X)	// 행
		{
			int newSectorX = x + X;
			int newSectorY = y + Y;

			// 새로운 인덱스가 범위내에 있는지 확인합니다
			if (newSectorX >= 0 && newSectorX < en_SECTOR_MAXIMUM_X &&
				newSectorY >= 0 && newSectorY < en_SECTOR_MAXIMUM_Y)
			{
				sectorAround.Around[index] = &Sectors[newSectorY][newSectorX];
				++sectorAround.SectorCount;
				++index;
			}
		}
	}
	return;
}

bool ChattingServer::Sector_Update(ChatUser* user, WORD SectorX, WORD SectorY)
{
	// 받은 섹터 위치 확인
	if (SectorX < 0 ||
		SectorX >= en_SECTOR_MAXIMUM_X ||
		SectorY < 0 ||
		SectorY >= en_SECTOR_MAXIMUM_Y)
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"Sector Range Out");
		return false;
	}

	// 현재 섹터에서 삭제
	Sector_UserErase(user);

	user->SectorX = SectorX;
	user->SectorY = SectorY;

	// 새로운 섹터에 삽입
	Sectors[SectorY][SectorX].push_back(user);
	return true;
}
void ChattingServer::Sector_SendAround(ChatUser* user, CPacket& packet, bool sendMe)
{
	SectorAround sectorAround;
	Sector_GetAround(user, sectorAround);

	if (sendMe)	// 자기 자신한테도 보내기
	{
		// pSession 주변 섹터에게 메시지 보내기
		for (int Idx = 0; Idx < sectorAround.SectorCount; ++Idx)
		{
			Sector* pSector = sectorAround.Around[Idx];

			// 섹터의 모든 클라이언트에게 메시지 보내기
			for (auto Iter = pSector->begin(); Iter != pSector->end(); ++Iter)
			{
				ChatUser* pOtherUser = *Iter;
				SendMSG_PQCS(pOtherUser->SessionID, packet);
			}
		}
	}
	else		// 자기 자신한테 안보내기
	{
		Sector* pUserSector = &Sectors[user->SectorY][user->SectorX];
		for (int Idx = 0; Idx < sectorAround.SectorCount; ++Idx)
		{
			// pSession이 속한 섹터일 때 패스(아래에서 따로 로직이 돕니다.)
			if (sectorAround.Around[Idx] == pUserSector)
				continue;

			// 섹터의 모든 클라이언트에게 메시지 보내기
			Sector* pSector = sectorAround.Around[Idx];
			for (auto iter = pSector->begin(); iter != pSector->end(); ++iter)
			{
				ChatUser* pOtherUser = *iter;
				SendMSG_PQCS(pOtherUser->SessionID, packet);
			}
		}

		// pSession이 속한 섹터의 모든 클라이언트에게 메시지 보내기
		for (auto iter = pUserSector->begin(); iter != pUserSector->end(); ++iter)
		{
			ChatUser* pOtherUser = *iter;
			if (user->SessionID == pOtherUser->SessionID)
				continue;
			SendMSG_PQCS(pOtherUser->SessionID, packet);
		}
	}

	return;
}
void ChattingServer::Sector_SendOne(ChatUser* user, CPacket& packet)
{
	// 섹터의 모든 클라이언트에게 메시지 보내기
	Sector* pSector = &Sectors[user->SectorY][user->SectorX];
	for (auto Iter = pSector->begin(); Iter != pSector->end(); ++Iter)
	{
		ChatUser* pUser = *Iter;
		SendMSG_PQCS(pUser->SessionID, packet);
	}
	return;
}