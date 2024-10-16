#include "ChattingServer2.h"

void ChattingServer2::Sector_GetAround(WORD x, WORD y, SectorAround& sectorAround)
{
	Sector* pSector = &sectors.sector[y][x];
	for (int iCnt = 0; iCnt < pSector->Around.SectorCount; ++iCnt)
	{
		sectorAround.Around[iCnt] = pSector->Around.Around[iCnt];
	}
	sectorAround.SectorCount = pSector->Around.SectorCount;

	return;
}
bool ChattingServer2::Sector_Update(ChatUser* user, const SectorPos& updatePos)
{
	// 받은 섹터 위치 확인
	if (updatePos.x < 0 ||
		updatePos.x >= en_SECTOR_MAXIMUM_X ||
		updatePos.y < 0 ||
		updatePos.y >= en_SECTOR_MAXIMUM_Y)
	{
		LOG(L"ChatServer", CLog::LEVEL_DEBUG, L"Sector Range Out");
		return false;
	}

	Sector* pDeleteSector = &sectors.sector[user->SectorY][user->SectorX];
	Sector* pUpdateSector = &sectors.sector[updatePos.y][updatePos.x];


	SectorAround SeqSector;
	if (pDeleteSector == pUpdateSector)
	{
		SeqSector.Around[0] = pUpdateSector;
		SeqSector.SectorCount = 1;
	}
	else
	{
		SeqSector.Around[0] = pDeleteSector;
		SeqSector.Around[1] = pUpdateSector;
		SeqSector.SectorCount = 2;
		Sector_Sort(SeqSector);
	}

	Sector_Lock(SeqSector);
	{
		// 현재 섹터에서 삭제
		pDeleteSector->remove(user);
		// 새로운 섹터에 삽입
		pUpdateSector->push_back(user);
		// 유저 섹터 위치 변환
		user->SectorX = updatePos.x;
		user->SectorY = updatePos.y;
	}
	Sector_UnLock(SeqSector);

	return true;
}
void ChattingServer2::Sector_Sort(SectorAround& seqSector)
{
	// pSectorArr Insert
	Sector* pSectorArr[12];
	for (int iCnt = 0; iCnt < seqSector.SectorCount; ++iCnt)
	{
		pSectorArr[iCnt] = seqSector.Around[iCnt];
	}

	// seqSector 순서 정렬 (Sectors에서 이미 2차원 배열로 만들어 놔서 Sector의 seq는 Sector 주소의 순서와 같다)
	std::sort(pSectorArr, pSectorArr + seqSector.SectorCount);

	// seqSector Insert
	for (int iCnt = 0; iCnt < seqSector.SectorCount; ++iCnt)
	{
		seqSector.Around[iCnt] = pSectorArr[iCnt];
	}

	return;
}
void ChattingServer2::Sector_Lock(const SectorAround& sectorAround)
{
	// 모든 섹터 Lock
	wstring lockseq = L"Lock-";
	for (int Idx = 0; Idx < sectorAround.SectorCount; ++Idx)
	{
		Sector* pSector = sectorAround.Around[Idx];
		EnterCriticalSection(&pSector->CS_Sector);
		lockseq += to_wstring(pSector->Seq);
		lockseq += L"-";
		if (InterlockedIncrement((unsigned long*)&pSector->count) != 1)
			int a = 1;
	}
	//OutputDebugStringF(L"%s\n", lockseq.c_str());
	return;
}
void ChattingServer2::Sector_UnLock(const SectorAround& sectorAround)
{
	// 모든 섹터 UnLock
	wstring lockseq = L"UnLock-";
	for (int Idx = 0; Idx < sectorAround.SectorCount; ++Idx)
	{
		Sector* pSector = sectorAround.Around[Idx];
		//OutputDebugStringF(L"unLock : %d\n", pSector->Seq);
		InterlockedDecrement((unsigned long*)&pSector->count);
		LeaveCriticalSection(&pSector->CS_Sector);
		lockseq += to_wstring(pSector->Seq);
		lockseq += L"-";
	}
	//OutputDebugStringF(L"%s\n", lockseq.c_str());

	return;
}
void ChattingServer2::Sector_UserErase(ChatUser* user, const SectorPos& Pos)
{
	if (Pos.x < 0 ||
		Pos.x >= en_SECTOR_MAXIMUM_X ||
		Pos.y < 0 ||
		Pos.y >= en_SECTOR_MAXIMUM_Y)
	{
		return;
	}

	Sector* pSector = &sectors.sector[Pos.y][Pos.x];
	SectorAround SeqSector;
	SeqSector.Around[0] = pSector;
	SeqSector.SectorCount = 1;

	Sector_Lock(SeqSector);
	pSector->remove(user);
	Sector_UnLock(SeqSector);

	return;
}

void ChattingServer2::Sector_SendAround(ChatUser* user, CPacket& packet, bool sendMe)
{
	CTlsProfile Profile(L"Sector_SendAround");
	SectorAround sectorAround;
	Sector_GetAround(user->SectorX, user->SectorY, sectorAround);
	Sector_Sort(sectorAround);

	Sector_Lock(sectorAround);
	if (sendMe)	// 자기 자신한테도 보내기
	{
		// pSession 주변 섹터에게 메시지 보내기
		for (int Idx = 0; Idx < sectorAround.SectorCount; ++Idx)
		{
			// 섹터의 모든 클라이언트에게 메시지 보내기
			Sector* pSector = sectorAround.Around[Idx];
			int x;
			int y;
			auto Iter = pSector->begin();
			if(Iter != pSector->end())
			{
				x = (*Iter)->SectorX;
				y = (*Iter)->SectorY;
			}
				
			int size = pSector->size();
			int count = 0;
			//for (auto Iter : *pSector)
			//{
			//	ChatUser* pOtherUser = Iter;
			//	SendMSG(pOtherUser->SessionID, packet);
			//	++count;
			//}
			for (auto Iter = pSector->begin(); Iter != pSector->end(); )
			{
				ChatUser* pOtherUser = *Iter;
				++Iter;
				SendMSG(pOtherUser->SessionID, packet);
				++count;
			}
		}
	}
	else		// 자기 자신한테 안보내기
	{
		//Sector* pUserSector = &sectors.sector[user->SectorY][user->SectorX];
		//for (int Idx = 0; Idx < sectorAround.SectorCount; ++Idx)
		//{
		//	// pSession이 속한 섹터일 때 패스(아래에서 따로 로직이 돕니다.)
		//	if (sectorAround.Around[Idx] == pUserSector)
		//		continue;

		//	// 섹터의 모든 클라이언트에게 메시지 보내기
		//	Sector* pSector = sectorAround.Around[Idx];
		//	for (auto iter = pSector->begin(); iter != pSector->end(); ++iter)
		//	{
		//		ChatUser* pOtherUser = *iter;
		//		SendMSG(pOtherUser->SessionID, packet);
		//	}
		//}

		//// pSession이 속한 섹터의 모든 클라이언트에게 메시지 보내기
		//for (auto iter = pUserSector->begin(); iter != pUserSector->end(); ++iter)
		//{
		//	ChatUser* pOtherUser = *iter;
		//	if (user->SessionID == pOtherUser->SessionID)
		//		continue;
		//	SendMSG(pOtherUser->SessionID, packet);
		//}
	}
	Sector_UnLock(sectorAround);


	return;
}
void ChattingServer2::Sector_SendOne(ChatUser* user, CPacket& packet)
{
	//CTlsProfile Profile(L"Sector_SendOne");
	//// 섹터의 모든 클라이언트에게 메시지 보내기
	//Sector* pSector = &sectors.sector[user->SectorY][user->SectorX];
	//EnterCriticalSection(&pSector->CS_Sector);
	//for (auto Iter = pSector->begin(); Iter != pSector->end(); ++Iter)
	//{
	//	ChatUser* pUser = *Iter;
	//	SendMSG(pUser->SessionID, packet);
	//}
	//LeaveCriticalSection(&pSector->CS_Sector);
	//return;
}


// 섹터의 이동이 될 때마다 주변 섹터에 생성, 삭제를 하는 섹터 이동처리 (MMOTCPFight 같은 섹터 처리)
void ChattingServer2::Sector_GetMoveSector(const SectorPos& currentPos, const SectorPos& updatePos, SectorAround& addSectors, SectorAround& removeSectors, SectorAround& seqSector)
{
	int CaseX = updatePos.x - currentPos.x;
	int CaseY = updatePos.y - currentPos.y;

	switch (CaseY) {
	case -1:
		switch (CaseX) {
		case -1:
			Sector_SetMoveSector(0, currentPos, updatePos, addSectors, removeSectors, seqSector);
			break;
		case 0:
			Sector_SetMoveSector(1, currentPos, updatePos, addSectors, removeSectors, seqSector);
			break;
		case 1:
			Sector_SetMoveSector(2, currentPos, updatePos, addSectors, removeSectors, seqSector);
			break;
		}
		break;
	case 0:
		switch (CaseX) {
		case -1:
			Sector_SetMoveSector(3, currentPos, updatePos, addSectors, removeSectors, seqSector);
			break;
		case 0:
			Sector_SetMoveSector(4, currentPos, updatePos, addSectors, removeSectors, seqSector);
			break;
		case 1:
			Sector_SetMoveSector(5, currentPos, updatePos, addSectors, removeSectors, seqSector);
			break;
		}
		break;
	case 1:
		switch (CaseX) {
		case -1:
			Sector_SetMoveSector(6, currentPos, updatePos, addSectors, removeSectors, seqSector);
			break;
		case 0:
			Sector_SetMoveSector(7, currentPos, updatePos, addSectors, removeSectors, seqSector);
			break;
		case 1:
			Sector_SetMoveSector(8, currentPos, updatePos, addSectors, removeSectors, seqSector);
			break;
		}
		break;
	}

	return;
}
// 섹터의 이동이 될 때마다 주변 섹터에 생성, 삭제를 하는 섹터 이동처리 (MMOTCPFight 같은 섹터 처리)
void ChattingServer2::Sector_SetMoveSector(WORD Case, const SectorPos currentPos, const SectorPos updatePos, SectorAround& addSectors, SectorAround& removeSectors, SectorAround& seqSector)
{
	DWORD Count;
	SectorPos AddSectorPos[9];
	SectorPos RemoveSectorPos[9];
	WORD OldSectorX = currentPos.x;
	WORD OldSectorY = currentPos.y;
	WORD NewSectorX = updatePos.x;
	WORD NewSectorY = updatePos.y;

	// AddSectorPos and RemoveSectorPos Settiong
	switch (Case)
	{
	case 0:
		Count = 5;
		AddSectorPos[0].x = NewSectorX - 1;
		AddSectorPos[0].y = NewSectorY - 1;
		AddSectorPos[1].x = NewSectorX;
		AddSectorPos[1].y = NewSectorY - 1;
		AddSectorPos[2].x = NewSectorX + 1;
		AddSectorPos[2].y = NewSectorY - 1;
		AddSectorPos[3].x = NewSectorX - 1;
		AddSectorPos[3].y = NewSectorY;
		AddSectorPos[4].x = NewSectorX - 1;
		AddSectorPos[4].y = NewSectorY + 1;

		RemoveSectorPos[0].x = OldSectorX + 1;
		RemoveSectorPos[0].y = OldSectorY - 1;
		RemoveSectorPos[0].x = OldSectorX + 1;
		RemoveSectorPos[0].y = OldSectorY;
		RemoveSectorPos[0].x = OldSectorX - 1;
		RemoveSectorPos[0].y = OldSectorY + 1;
		RemoveSectorPos[0].x = OldSectorX;
		RemoveSectorPos[0].y = OldSectorY + 1;
		RemoveSectorPos[0].x = OldSectorX + 1;
		RemoveSectorPos[0].y = OldSectorY + 1;
		break;
	case 1:
		Count = 3;
		AddSectorPos[0].x = NewSectorX - 1;
		AddSectorPos[0].y = NewSectorY - 1;
		AddSectorPos[1].x = NewSectorX;
		AddSectorPos[1].y = NewSectorY - 1;
		AddSectorPos[2].x = NewSectorX + 1;
		AddSectorPos[2].y = NewSectorY - 1;

		RemoveSectorPos[0].x = OldSectorX - 1;
		RemoveSectorPos[0].y = OldSectorY + 1;
		RemoveSectorPos[1].x = OldSectorX;
		RemoveSectorPos[1].y = OldSectorY + 1;
		RemoveSectorPos[2].x = OldSectorX + 1;
		RemoveSectorPos[2].y = OldSectorY + 1;
		break;
	case 2:
		Count = 5;
		AddSectorPos[0].x = NewSectorX - 1;
		AddSectorPos[0].y = NewSectorY - 1;
		AddSectorPos[1].x = NewSectorX;
		AddSectorPos[1].y = NewSectorY - 1;
		AddSectorPos[2].x = NewSectorX + 1;
		AddSectorPos[2].y = NewSectorY - 1;
		AddSectorPos[3].x = NewSectorX + 1;
		AddSectorPos[3].y = NewSectorY;
		AddSectorPos[4].x = NewSectorX + 1;
		AddSectorPos[4].y = NewSectorY + 1;

		RemoveSectorPos[0].x = OldSectorX - 1;
		RemoveSectorPos[0].y = OldSectorY - 1;
		RemoveSectorPos[1].x = OldSectorX - 1;
		RemoveSectorPos[1].y = OldSectorY;
		RemoveSectorPos[2].x = OldSectorX - 1;
		RemoveSectorPos[2].y = OldSectorY + 1;
		RemoveSectorPos[3].x = OldSectorX;
		RemoveSectorPos[3].y = OldSectorY + 1;
		RemoveSectorPos[4].x = OldSectorX + 1;
		RemoveSectorPos[4].y = OldSectorY + 1;
		break;
	case 3:
		Count = 3;
		AddSectorPos[0].x = NewSectorX - 1;
		AddSectorPos[0].y = NewSectorY - 1;
		AddSectorPos[1].x = NewSectorX - 1;
		AddSectorPos[1].y = NewSectorY;
		AddSectorPos[2].x = NewSectorX - 1;
		AddSectorPos[2].y = NewSectorY + 1;

		RemoveSectorPos[0].x = OldSectorX + 1;
		RemoveSectorPos[0].y = OldSectorY - 1;
		RemoveSectorPos[1].x = OldSectorX + 1;
		RemoveSectorPos[1].y = OldSectorY;
		RemoveSectorPos[2].x = OldSectorX + 1;
		RemoveSectorPos[2].y = OldSectorY + 1;
		break;
	case 4:
		Count = 0;
		break;
	case 5:
		Count = 3;
		AddSectorPos[0].x = NewSectorX + 1;
		AddSectorPos[0].y = NewSectorY - 1;
		AddSectorPos[1].x = NewSectorX + 1;
		AddSectorPos[1].y = NewSectorY;
		AddSectorPos[2].x = NewSectorX + 1;
		AddSectorPos[2].y = NewSectorY + 1;

		RemoveSectorPos[0].x = OldSectorX - 1;
		RemoveSectorPos[0].y = OldSectorY - 1;
		RemoveSectorPos[1].x = OldSectorX - 1;
		RemoveSectorPos[1].y = OldSectorY;
		RemoveSectorPos[2].x = OldSectorX - 1;
		RemoveSectorPos[2].y = OldSectorY + 1;
		break;
	case 6:
		Count = 5;
		AddSectorPos[0].x = NewSectorX - 1;
		AddSectorPos[0].y = NewSectorY - 1;
		AddSectorPos[1].x = NewSectorX - 1;
		AddSectorPos[1].y = NewSectorY;
		AddSectorPos[2].x = NewSectorX - 1;
		AddSectorPos[2].y = NewSectorY + 1;
		AddSectorPos[3].x = NewSectorX;
		AddSectorPos[3].y = NewSectorY + 1;
		AddSectorPos[4].x = NewSectorX + 1;
		AddSectorPos[4].y = NewSectorY + 1;

		RemoveSectorPos[0].x = OldSectorX - 1;
		RemoveSectorPos[0].y = OldSectorY - 1;
		RemoveSectorPos[1].x = OldSectorX;
		RemoveSectorPos[1].y = OldSectorY - 1;
		RemoveSectorPos[2].x = OldSectorX + 1;
		RemoveSectorPos[2].y = OldSectorY - 1;
		RemoveSectorPos[3].x = OldSectorX + 1;
		RemoveSectorPos[3].y = OldSectorY;
		RemoveSectorPos[4].x = OldSectorX + 1;
		RemoveSectorPos[4].y = OldSectorY + 1;
		break;
	case 7:
		Count = 3;
		AddSectorPos[0].x = NewSectorX - 1;
		AddSectorPos[0].y = NewSectorY + 1;
		AddSectorPos[1].x = NewSectorX;
		AddSectorPos[1].y = NewSectorY + 1;
		AddSectorPos[2].x = NewSectorX + 1;
		AddSectorPos[2].y = NewSectorY + 1;

		RemoveSectorPos[0].x = OldSectorX - 1;
		RemoveSectorPos[0].y = OldSectorY - 1;
		RemoveSectorPos[1].x = OldSectorX;
		RemoveSectorPos[1].y = OldSectorY - 1;
		RemoveSectorPos[2].x = OldSectorX + 1;
		RemoveSectorPos[2].y = OldSectorY - 1;
		break;
	case 8:
		Count = 5;
		AddSectorPos[0].x = NewSectorX + 1;
		AddSectorPos[0].y = NewSectorY - 1;
		AddSectorPos[1].x = NewSectorX + 1;
		AddSectorPos[1].y = NewSectorY;
		AddSectorPos[2].x = NewSectorX - 1;
		AddSectorPos[2].y = NewSectorY + 1;
		AddSectorPos[3].x = NewSectorX;
		AddSectorPos[3].y = NewSectorY + 1;
		AddSectorPos[4].x = NewSectorX + 1;
		AddSectorPos[4].y = NewSectorY + 1;

		RemoveSectorPos[0].x = OldSectorX - 1;
		RemoveSectorPos[0].y = OldSectorY - 1;
		RemoveSectorPos[1].x = OldSectorX;
		RemoveSectorPos[1].y = OldSectorY - 1;
		RemoveSectorPos[2].x = OldSectorX + 1;
		RemoveSectorPos[2].y = OldSectorY - 1;
		RemoveSectorPos[3].x = OldSectorX - 1;
		RemoveSectorPos[3].y = OldSectorY;
		RemoveSectorPos[4].x = OldSectorX - 1;
		RemoveSectorPos[4].y = OldSectorY + 1;
		break;
	}

	// addSectors Insert
	int AddIndex = 0;
	for (int iCnt = 0; iCnt < Count; ++iCnt)
	{
		if (AddSectorPos[iCnt].x >= 0 &&
			AddSectorPos[iCnt].x < en_SECTOR_MAXIMUM_X &&
			AddSectorPos[iCnt].y >= 0 &&
			AddSectorPos[iCnt].y < en_SECTOR_MAXIMUM_Y)
		{
			addSectors.Around[AddIndex] = &sectors.sector[AddSectorPos[iCnt].y][AddSectorPos[iCnt].x];
			++addSectors.SectorCount;
			++AddIndex;
		}
	}
	// removeSectors Insert
	int RemoveIndex = 0;
	for (int iCnt = 0; iCnt < Count; ++iCnt)
	{
		if (RemoveSectorPos[iCnt].x >= 0 &&
			RemoveSectorPos[iCnt].x < en_SECTOR_MAXIMUM_X &&
			RemoveSectorPos[iCnt].y >= 0 &&
			RemoveSectorPos[iCnt].y < en_SECTOR_MAXIMUM_Y)
		{
			removeSectors.Around[RemoveIndex] = &sectors.sector[RemoveSectorPos[iCnt].y][RemoveSectorPos[iCnt].x];
			++removeSectors.SectorCount;
			++RemoveIndex;
		}
	}


	// pSectorArr Insert
	Sector* pSectorArr[12];
	int ArrIndex = 0;
	pSectorArr[ArrIndex++] = &sectors.sector[OldSectorY][OldSectorX];
	pSectorArr[ArrIndex++] = &sectors.sector[NewSectorY][NewSectorX];
	// pSectorArr에 addSectors 삽입
	for (int iCnt = 0; iCnt < addSectors.SectorCount; ++iCnt)
	{
		pSectorArr[ArrIndex++] = addSectors.Around[iCnt];
	}
	// seqSector에 removeSectors 삽입
	for (int iCnt = 0; iCnt < removeSectors.SectorCount; ++iCnt)
	{
		pSectorArr[ArrIndex++] = removeSectors.Around[iCnt];
	}
	// seqSector 순서 정렬 (Sectors에서 이미 2차원 배열로 만들어 놔서 Sector의 seq는 Sector 주소의 순서와 같다)
	std::sort(pSectorArr, pSectorArr + seqSector.SectorCount);

	// seqSector Insert
	for (int iCnt = 0; iCnt < ArrIndex; ++iCnt)
	{
		seqSector.Around[iCnt] = pSectorArr[iCnt];
	}
	seqSector.SectorCount = ArrIndex;

	return;
}