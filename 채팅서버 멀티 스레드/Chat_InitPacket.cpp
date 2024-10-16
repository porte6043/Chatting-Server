#include "ChattingServer2.h"

void ChattingServer2::InitPacket_ResLogin(CPacket& packet, BYTE Status, INT64 AccountNo)
{
	//------------------------------------------------------------
	// 채팅서버 로그인 응답
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status				// 0:실패	1:성공
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
	// 채팅서버 섹터 이동 결과
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
// 채팅서버 채팅보내기 응답  (다른 클라가 보낸 채팅도 이걸로 받음)
//
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		WCHAR	ID[20]						// null 포함
//		WCHAR	Nickname[20]				// null 포함
//		
//		WORD	MessageLen
//		WCHAR	Message[MessageLen / 2]		// null 미포함
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