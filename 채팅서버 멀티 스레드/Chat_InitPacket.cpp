#include "ChattingServer2.h"

void ChattingServer2::InitPacket_ResLogin(CPacket& packet, BYTE Status, INT64 AccountNo)
{
	//------------------------------------------------------------
	// ä�ü��� �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status				// 0:����	1:����
	//		INT64	AccountNo
	//	}
	//
	//------------------------------------------------------------
	packet.clear();
	WORD Type = en_PACKET_CS_CHAT_RES_LOGIN;
	packet << Type << Status << AccountNo;
	return;
}
void ChattingServer2::InitPacket_ResSectorMove(CPacket& packet, INT64 AccountNo, WORD SectorX, WORD SectorY)
{
	//------------------------------------------------------------
	// ä�ü��� ���� �̵� ���
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	packet.clear();
	WORD Type = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	packet << Type << AccountNo << SectorX << SectorY;
	return;
}
void ChattingServer2::InitPacket_ResMessage(CPacket& packet, INT64 AccountNo, const WCHAR* ID, const WCHAR* Nickname, WORD MessageLen, const WCHAR* Message)
{
	//------------------------------------------------------------
// ä�ü��� ä�ú����� ����  (�ٸ� Ŭ�� ���� ä�õ� �̰ɷ� ����)
//
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		WCHAR	ID[20]						// null ����
//		WCHAR	Nickname[20]				// null ����
//		
//		WORD	MessageLen
//		WCHAR	Message[MessageLen / 2]		// null ������
//	}
//
//------------------------------------------------------------
	packet.clear();
	WORD Type = en_PACKET_CS_CHAT_RES_MESSAGE;
	packet << Type << AccountNo;
	packet.PutData(ID, en_LEN_ID * 2);
	packet.PutData(Nickname, en_LEN_NICKNAME * 2);
	packet << MessageLen;
	packet.PutData(Message, MessageLen);
	return;
}