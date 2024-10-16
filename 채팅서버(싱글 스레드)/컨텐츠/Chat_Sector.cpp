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

	//for(auto &i : Sectors[user->SectorY][user->SectorX]) // ���� for��

	return;
}
void ChattingServer::Sector_GetAround(ChatUser* user, SectorAround& sectorAround)
{
	int index = 0;
	for (int Y = -1; Y <= 1; ++Y) // ��
	{
		for (int X = -1; X <= 1; ++X)	// ��
		{
			int newSectorX = user->SectorX + X;
			int newSectorY = user->SectorY + Y;

			// ���ο� �ε����� �������� �ִ��� Ȯ���մϴ�
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
	for (int Y = -1; Y <= 1; ++Y) // ��
	{
		for (int X = -1; X <= 1; ++X)	// ��
		{
			int newSectorX = x + X;
			int newSectorY = y + Y;

			// ���ο� �ε����� �������� �ִ��� Ȯ���մϴ�
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
	// ���� ���� ��ġ Ȯ��
	if (SectorX < 0 ||
		SectorX >= en_SECTOR_MAXIMUM_X ||
		SectorY < 0 ||
		SectorY >= en_SECTOR_MAXIMUM_Y)
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"Sector Range Out");
		return false;
	}

	// ���� ���Ϳ��� ����
	Sector_UserErase(user);

	user->SectorX = SectorX;
	user->SectorY = SectorY;

	// ���ο� ���Ϳ� ����
	Sectors[SectorY][SectorX].push_back(user);
	return true;
}
void ChattingServer::Sector_SendAround(ChatUser* user, CPacket& packet, bool sendMe)
{
	SectorAround sectorAround;
	Sector_GetAround(user, sectorAround);

	if (sendMe)	// �ڱ� �ڽ����׵� ������
	{
		// pSession �ֺ� ���Ϳ��� �޽��� ������
		for (int Idx = 0; Idx < sectorAround.SectorCount; ++Idx)
		{
			Sector* pSector = sectorAround.Around[Idx];

			// ������ ��� Ŭ���̾�Ʈ���� �޽��� ������
			for (auto Iter = pSector->begin(); Iter != pSector->end(); ++Iter)
			{
				ChatUser* pOtherUser = *Iter;
				SendMSG_PQCS(pOtherUser->SessionID, packet);
			}
		}
	}
	else		// �ڱ� �ڽ����� �Ⱥ�����
	{
		Sector* pUserSector = &Sectors[user->SectorY][user->SectorX];
		for (int Idx = 0; Idx < sectorAround.SectorCount; ++Idx)
		{
			// pSession�� ���� ������ �� �н�(�Ʒ����� ���� ������ ���ϴ�.)
			if (sectorAround.Around[Idx] == pUserSector)
				continue;

			// ������ ��� Ŭ���̾�Ʈ���� �޽��� ������
			Sector* pSector = sectorAround.Around[Idx];
			for (auto iter = pSector->begin(); iter != pSector->end(); ++iter)
			{
				ChatUser* pOtherUser = *iter;
				SendMSG_PQCS(pOtherUser->SessionID, packet);
			}
		}

		// pSession�� ���� ������ ��� Ŭ���̾�Ʈ���� �޽��� ������
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
	// ������ ��� Ŭ���̾�Ʈ���� �޽��� ������
	Sector* pSector = &Sectors[user->SectorY][user->SectorX];
	for (auto Iter = pSector->begin(); Iter != pSector->end(); ++Iter)
	{
		ChatUser* pUser = *Iter;
		SendMSG_PQCS(pUser->SessionID, packet);
	}
	return;
}