#include "CNetwork.h"

CNETWORK::CNETWORK() : SessionIndex(), CS_SessionIndex(), hTimers{}, TimerList{}
{
	Sessions = nullptr;
	TimerCount = 0;
	ListenSocket = INVALID_SOCKET;
	Port = 0;
	MaxSessionCount = 0;
	NumberOfCore = 0;
	hCP = nullptr;
	hAccept_Threads = nullptr;
	hTimer_Threads = nullptr;
	hWorker_Threads = nullptr;
}
CNETWORK::~CNETWORK()
{

}

bool CNETWORK::Start(const WCHAR* IP, WORD Port, WORD NumberOfCreateThreads, WORD NumberOfConcurrentThreads, bool Nagle, WORD MaxSessionCount)
{
	// 콘솔 초기화
	cs_Initial();

	// wprintf의 한글 사용
	_wsetlocale(LC_ALL, L"korean");

	// CS 구조체 초기화
	InitializeCriticalSection(&CS_SessionIndex);

	this->Port = Port;
	this->MaxSessionCount = MaxSessionCount;

	//Session 생성
	Sessions = new Session[MaxSessionCount];

	// SessionArr의 비어있는 인덱스 넣기
	for (int iCnt = MaxSessionCount - 1; iCnt >= 0; --iCnt)
	{
		SessionIndex.push(iCnt);
	}

	// 입출력 완료 포트 생성
	hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, NumberOfConcurrentThreads);
	if (hCP == NULL)
	{
		int Err = GetLastError();
		LOG(L"Network", CLog::LEVEL_ERROR, L"Fail CreateIoCompletionPort() ErrorCode : %d", Err);
		return 1;
	}

	// IOCP 워커스레드 생성
	hWorker_Threads = new HANDLE[NumberOfCreateThreads];
	for (unsigned int iCnt = 0; iCnt < NumberOfCreateThreads; ++iCnt)
	{
		hWorker_Threads[iCnt] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThreadWrapping, this, 0, nullptr);
		if (hWorker_Threads[iCnt] == 0)
		{
			int Err = errno;
			LOG(L"Network", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
			return false;
		}
	}

	// listen socket 초기화
	ListenSocketInit(IP, Port, Nagle);
	// Accept 스레드 생성
	hAccept_Threads = (HANDLE)_beginthreadex(nullptr, 0, AcceptThreadWrapping, this, 0, nullptr);
	if (hAccept_Threads == 0)
	{
		int Err = errno;
		LOG(L"Network", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
		return false;
	}

	// 타이머 스레드 생성
	hTimer_Threads = (HANDLE)_beginthreadex(nullptr, 0, TimerThreadWrapping, this, 0, nullptr);
	if (hTimer_Threads == 0)
	{
		int Err = errno;
		LOG(L"Network", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
		return false;
	}

	return true;

}

bool CNETWORK::Disconnect(DWORD64 SessionID)
{
	Session* pSession = SessionFind(SessionID);
	int retIOCount = InterlockedIncrement(&pSession->IOCount);

	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}
	else
	{
		if (SessionID != pSession->SessionID)	// 검색한 후 SessionID에 해당하는 세션이 Release되어 다른 세션으로 사용된 경우
		{
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			return false;
		}
	}

	InterlockedExchange(&pSession->DisconnectFlag, 1);
	CancelIoEx(reinterpret_cast<HANDLE>(pSession->socket), NULL);

	if (0 == InterlockedDecrement(&pSession->IOCount))
		SessionRelease(pSession);

	return true;
}
void CNETWORK::Disconnect(Session* pSession)
{
	InterlockedExchange(&pSession->DisconnectFlag, 1);
	CancelIoEx(reinterpret_cast<HANDLE>(pSession->socket), NULL);

	return;
}


// PQCS 버전
bool CNETWORK::SendMSG(DWORD64 SessionID, CPacket& packet)
{
	Session* pSession = SessionFind(SessionID);
	int retIOCount = InterlockedIncrement(&pSession->IOCount);
	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}
	else
	{
		if (SessionID != pSession->SessionID)	// 검색한 후 SessionID에 해당하는 세션이 Release되어 다른 세션으로 사용된 경우
		{
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			return false;
		}
	}

	if (pSession->DisconnectFlag == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}

	SerialRingBuffer* Packet = packet.Packet;
	Network_Header NetworkHeader(Packet);
	Packet->SetHeader(&NetworkHeader, sizeof(Network_Header));
	Packet->encode();

	Packet->AddRef();
	EnterCriticalSection(&pSession->CS_SendQ);
	pSession->SendQ.enqueue(&Packet, sizeof(SerialRingBuffer*));
	LeaveCriticalSection(&pSession->CS_SendQ);

	PostQueuedCompletionStatus(hCP, 0, (ULONG_PTR)pSession, (LPOVERLAPPED)1);

	// Monitor용
	InterlockedIncrement(&Pnd.SendTPS);
	return true;
}

int CNETWORK::GetSessionCount()
{
	return MaxSessionCount - SessionIndex.size();
}

void CNETWORK::GetPND(PerformanceNetworkData& pnd)
{
	pnd.SessionCount = GetSessionCount();
	pnd.AcceptTotalCount = Pnd.AcceptTotalCount;
	pnd.PacketTotalCount = SerialRingBuffer::GetPoolTotalCount();
	pnd.PacketUseCount = SerialRingBuffer::GetPoolUseCount();
	pnd.AcceptTPS = InterlockedExchange(&Pnd.AcceptTPS, 0);
	pnd.SendTPS = InterlockedExchange(&Pnd.SendTPS, 0);
	pnd.RecvTPS = InterlockedExchange(&Pnd.RecvTPS, 0);
	return;
}

int CNETWORK::GetPort()
{
	return Port;
}

int CNETWORK::GetAcceptTPS()
{
	return InterlockedExchange(&Pnd.AcceptTPS, 0);
}

int CNETWORK::GetRecvTPS()
{
	return InterlockedExchange(&Pnd.RecvTPS, 0);
}

int CNETWORK::GetSendTPS()
{
	return InterlockedExchange(&Pnd.SendTPS, 0);
}

int CNETWORK::GetTotalAcceptCount()
{
	return Pnd.AcceptTotalCount;
}

bool CNETWORK::ListenSocketInit(const WCHAR* IP, WORD Port, bool Nagle)
{
	ULONG Ip;
	InetPton(AF_INET, IP, &Ip);

	// 윈속 초기화
	WSADATA WSA;
	if (WSAStartup(MAKEWORD(2, 2), &WSA) != 0)
	{
		int Err = WSAGetLastError();
		LOG(L"Network", CLog::LEVEL_ERROR,
			L"Fail WSAStartup() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network", CLog::LEVEL_SYSTEM, L"WSAStartup()");


	// Listen 소켓 생성
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET)
	{
		int Err = WSAGetLastError();
		LOG(L"Network", CLog::LEVEL_ERROR,
			L"Fail ListenSocket() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network", CLog::LEVEL_SYSTEM, L"ListenSocket()");

	//네이글 알고리즘 on/off
	int option = Nagle;
	setsockopt(ListenSocket,  //해당 소켓
		IPPROTO_TCP,          //소켓의 레벨
		TCP_NODELAY,          //설정 옵션
		(const char*)&option, // 옵션 포인터
		sizeof(option));      //옵션 크기

	// Listen 소켓 주소 초기화
	SOCKADDR_IN ListenSockAddr;
	ZeroMemory(&ListenSockAddr, sizeof(ListenSockAddr));
	ListenSockAddr.sin_family = AF_INET;
	ListenSockAddr.sin_addr.S_un.S_addr = htonl(Ip);
	ListenSockAddr.sin_port = htons(Port);

	// 바인딩
	if (bind(ListenSocket, (sockaddr*)&ListenSockAddr, sizeof(ListenSockAddr)) == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		LOG(L"Network", CLog::LEVEL_ERROR,
			L"Fail bind() ErrorCode : %d", err);
		return false;
	}
	LOG(L"Network", CLog::LEVEL_SYSTEM, L"bind() Port:%d", Port);

	// listen()
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		LOG(L"Network", CLog::LEVEL_ERROR,
			L"Fail listen() ErrorCode : %d", err);
		return false;
	}
	LOG(L"Network", CLog::LEVEL_SYSTEM, L"listen()");

	return true;
}

void CNETWORK::ClientInfo(Session* pSession, WCHAR* IP, WORD* Port)
{
	// 클라이언트 정보 얻기
	SOCKADDR_IN ClientAddr;
	int AddrLen = sizeof(ClientAddr);
	getpeername(pSession->socket, (SOCKADDR*)&ClientAddr, &AddrLen);

	// IP와 port 세팅
	*Port = ntohs(ClientAddr.sin_port);
	InetNtop(AF_INET, &ClientAddr.sin_addr, IP, 16);

	return;
}

void CNETWORK::ClientInfo(DWORD64 SessionID, WCHAR* IP, WORD* Port)
{
	Session* pSession = SessionFind(SessionID);
	int retIOCount = InterlockedIncrement(&pSession->IOCount);

	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return;
	}
	else
	{
		if (SessionID != pSession->SessionID)	// 검색한 후 SessionID에 해당하는 세션이 Release되어 다른 세션으로 사용된 경우
		{
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			return;
		}
	}

	// 클라이언트 정보 얻기
	SOCKADDR_IN ClientAddr;
	int AddrLen = sizeof(ClientAddr);
	getpeername(pSession->socket, (SOCKADDR*)&ClientAddr, &AddrLen);

	// IP와 port 세팅
	*Port = ntohs(ClientAddr.sin_port);
	InetNtop(AF_INET, &ClientAddr.sin_addr, IP, 16);

	if (0 == InterlockedDecrement(&pSession->IOCount))
		SessionRelease(pSession);
	return;
}

void CNETWORK::NetworkError(int Err, const WCHAR* FunctionName)
{
	//Err == WSAEINVAL				// 10022 : WSASend에서 dwBufferCount 인수에 '0'이 들어갔을 떄 나타나는 것을 확인

	if (Err != WSAECONNRESET &&			// 10054 : 클라 측에서 끊었을 경우
		Err != WSAECONNABORTED &&		// 10053 : 소프트웨어(TCP)로 연결이 중단된 경우
		Err != ERROR_NETNAME_DELETED &&	// 64	 : RST, 클라에서 종료 했을 경우
		Err != WSAENOTSOCK &&			// 10038 : 소켓이 아닌 항목에서 작업을 시도했습니다.
		Err != ERROR_OPERATION_ABORTED	// 995	 : 스레드 종료 또는 애플리케이션 요청으로 인해 I/O 작업이 중단되었습니다. (CancelIoEx)
		)
	{
		LOG(L"Network", CLog::LEVEL_ERROR, L"Fail %s ErrorCode : %d", FunctionName, Err);
	}
	return;
}

void CNETWORK::NetworkMessageProc(Session* pSession)
{
	while (1)
	{
		// NetworkHeader 보다 크게 있는지 확인
		if (sizeof(Network_Header) >= pSession->RecvQ.GetUseSize())
			break;

		// Network Header peek
		Network_Header NetworkHeader;
		int retpeek = pSession->RecvQ.peek(&NetworkHeader, sizeof(Network_Header));

		// code 확인
		if (NetworkHeader.Code != df_PACKET_CODE)
		{
			LOG(L"Network", CLog::LEVEL_DEBUG, L"Packet Error : Code Mismatch");
			Disconnect(pSession->SessionID);
			break;
		}

		// PayLoad가 버퍼 크기보다 큰지 확인
		if (NetworkHeader.Len > SerialRingBuffer::en_BUFFER_DEFAULT_SIZE - sizeof(Network_Header) - 1)
		{
			LOG(L"Network", CLog::LEVEL_DEBUG, L"Packet Error : Buffer Size Overflow");
			Disconnect(pSession->SessionID);
			break;
		}

		// PayLoad의 크기가 0 인 경우
		if (NetworkHeader.Len == 0)
		{
			LOG(L"Network", CLog::LEVEL_DEBUG, L"Packet Error : No PayLoad");
			Disconnect(pSession->SessionID);
			break;
		}


		// PayLoad가 들어왔는지 확인
		if (pSession->RecvQ.GetUseSize() - sizeof(Network_Header) >= NetworkHeader.Len)
		{
			// Network 헤더 크기만큼 이동
			pSession->RecvQ.MoveFront(sizeof(Network_Header));

			SerialRingBuffer* Packet = SerialRingBuffer::Alloc();
			Packet->SetHeader(&NetworkHeader, sizeof(Network_Header));
			int retDeq = pSession->RecvQ.dequeue(Packet->buffer(), NetworkHeader.Len);
			if (retDeq != NetworkHeader.Len)
			{
				LOG(L"Nework", CLog::LEVEL_ERROR, L"Failed dequeue()");
				Crash();
			}
			Packet->MoveRear(retDeq);

			// decode 실패
			if (!Packet->decode())
			{
				LOG(L"Network", CLog::LEVEL_DEBUG, L"Failed decode()");
				Packet->Free();
				Disconnect(pSession->SessionID);
				break;
			}


			CPacket packet(Packet);
			OnRecv(pSession->SessionID, packet);

			// Monitor 용
			InterlockedIncrement(&Pnd.RecvTPS);
		}
		else break;
	}

	return;
}

void CNETWORK::RecvPost(Session* pSession)
{
	// 비동기 입출력 시작
	DWORD Flags = 0;
	WSABUF WsaBuf[2];
	WsaBuf[0].buf = pSession->RecvQ.rear();
	WsaBuf[0].len = pSession->RecvQ.DirectEnqueueSize();
	WsaBuf[1].buf = pSession->RecvQ.buffer();
	WsaBuf[1].len = pSession->RecvQ.GetFreeSize() - pSession->RecvQ.DirectEnqueueSize();
	ZeroMemory(&pSession->RecvOverlapped, sizeof(OVERLAPPED));

	InterlockedIncrement(&pSession->IOCount);
	int retRecv = WSARecv(pSession->socket, WsaBuf, 2, nullptr, &Flags, (OVERLAPPED*)&pSession->RecvOverlapped, NULL);
	if (retRecv == SOCKET_ERROR)
	{
		int Err = WSAGetLastError();
		if (Err != ERROR_IO_PENDING)
		{
			NetworkError(Err, L"WSARecv");
			// WSARecv를 실패 했을 경우 IOCount 감소
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
		}
		else
		{
			if (pSession->DisconnectFlag == 1)
				CancelIoEx(reinterpret_cast<HANDLE>(pSession->socket), NULL);
		}
	}

	if (0 == InterlockedDecrement(&pSession->IOCount))
		SessionRelease(pSession);
	return;
}

void CNETWORK::SendPost(Session* pSession)
{
	/*
	Send의 1회 제한
	1회로 제한 하지 않으면 WSASend 완료통지가 와서 링버퍼의 front가 처리 되기 전에 WSARecv 완료통지가
	먼저 와서 메시지 처리 후 WSARecv완료통지 쪽에서 다시 한번 WSASend를 하게 되면 이전 WSASend의 front가
	처리 되기 전에 Send를 하게 되어 메시지가 중첩된게 보내질 수 있다.
	*/
	CTlsProfile Profile(L"SendPost");
	SerialRingBuffer* SendQ = &pSession->SendQ;

	/*
		만약 지역변수로 UseSize를 사용하지 않으면, SendMSG로 인해 SendPost가 호출 되면 Session에 lock이 걸려있어서 상관 없지만
		WorkerThread에서 Send완료통지에서 SendPost를 호출 하는 경우 Session에 lock이 걸려있지 않기 때문에 모든 라인에서 GetUs
		esize()의 크기가 달라질 수 있다.
	*/
	while (1)
	{
		if (1 == InterlockedExchange(&pSession->SendFlag, 1))
			return;

		if (SendQ->GetUseSize() != 0)
		{
			break;
		}
		else
		{
			InterlockedExchange(&pSession->SendFlag, 0);
			if (SendQ->GetUseSize() != 0)
				continue;
			else
				return;
		}
	}

	// SerialRingBuffer의 point를 SendQ에 넣습니다.
	WSABUF WsaBuf[800];
	pSession->SendBufferCount = SendQ->GetUseSize() / 8;
	unsigned int DirectSize = pSession->SendQ.DirectDequeueSize();
	if (DirectSize / 8 >= pSession->SendBufferCount)
	{
		for (unsigned int iCnt = 0; iCnt < pSession->SendBufferCount; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(SendQ->front() + iCnt * 8));

			WsaBuf[iCnt].buf = pPacket->head();
			WsaBuf[iCnt].len = pPacket->GetUseSize() + sizeof(Network_Header);
		}
		ZeroMemory(&pSession->SendOverlapped, sizeof(OVERLAPPED));
	}
	else
	{
		int index = 0;
		int iCnt_1 = DirectSize / 8;
		int iCnt_2 = pSession->SendBufferCount - iCnt_1 - 1;

		for (int iCnt = 0; iCnt < iCnt_1; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(SendQ->front() + iCnt * 8));
			WsaBuf[index].buf = pPacket->head();
			WsaBuf[index].len = pPacket->GetUseSize() + sizeof(Network_Header);
			++index;
		}

		char Buffer[8];
		memcpy(Buffer, SendQ->front() + iCnt_1 * 8, DirectSize - iCnt_1 * 8);
		memcpy(Buffer + DirectSize - iCnt_1 * 8, SendQ->buffer(), 8 - (DirectSize - iCnt_1 * 8));
		SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)Buffer);
		WsaBuf[index].buf = pPacket->head();
		WsaBuf[index].len = pPacket->GetUseSize() + sizeof(Network_Header);
		++index;

		char* front = SendQ->buffer() + 8 - (DirectSize - iCnt_1 * 8);
		for (int iCnt = 0; iCnt < iCnt_2; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(front + iCnt * 8));
			WsaBuf[index].buf = pPacket->head();
			WsaBuf[index].len = pPacket->GetUseSize() + sizeof(Network_Header);
			++index;
		}

		ZeroMemory(&pSession->SendOverlapped, sizeof(OVERLAPPED));
	}

	// IOCount의 증가
	if (1 == InterlockedIncrement(&pSession->IOCount))
	{
		DebugBreak();
		return;
	}

	int retSend = WSASend(pSession->socket, WsaBuf, pSession->SendBufferCount, nullptr, 0, (OVERLAPPED*)&pSession->SendOverlapped, NULL);
	if (retSend == SOCKET_ERROR)
	{
		int Err = WSAGetLastError();
		if (Err != WSA_IO_PENDING)
		{
			NetworkError(Err, L"WSASend");

			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
		}
	}

	return;
}

void CNETWORK::SessionRelease(Session* pSession)
{
	CTlsProfile Profile(L"SessionRelease");
	DWORD CompareIOCount = IOCOUNT(0, 0);
	DWORD ExchangeIOCount = IOCOUNT(1, 0);
	DWORD retIOCount = InterlockedCompareExchange(&pSession->IOCount, ExchangeIOCount, CompareIOCount);
	if (CompareIOCount != retIOCount)
		return;

	DWORD64 SessionID = SessionFree(pSession);

	OnSessionRelease(SessionID);

	return;
}

CNETWORK::Session* CNETWORK::SessionFind(DWORD64 SessionID)
{
	return &Sessions[SESSIONID_INDEX_LOW(SessionID)];
}

CNETWORK::Session* CNETWORK::SessionAlloc(SOCKET Socket, DWORD64 sessionID)
{
	while (SessionIndex.empty())
	{
		Sleep(1000);
	}

	EnterCriticalSection(&CS_SessionIndex);
	DWORD Index = SessionIndex.top();
	SessionIndex.pop();
	LeaveCriticalSection(&CS_SessionIndex);


	Sessions[Index].socket = Socket;
	Sessions[Index].SessionID = SESSIONID(sessionID, Index);
	Sessions[Index].SendFlag = 0;
	Sessions[Index].DisconnectFlag = 0;
	Sessions[Index].SendBufferCount = 0;
	Sessions[Index].RecvQ.clear();
	Sessions[Index].SendQ.clear();
	ZeroMemory(&Sessions[Index].RecvOverlapped, sizeof(OVERLAPPED));
	ZeroMemory(&Sessions[Index].SendOverlapped, sizeof(OVERLAPPED));
	InterlockedIncrement(&Sessions[Index].IOCount);
	InterlockedAnd((long*)&Sessions[Index].IOCount, 0x0000'ffff);

	return &Sessions[Index];
}

DWORD64 CNETWORK::SessionFree(Session* pSession)
{
	closesocket(pSession->socket);

	DWORD64 SessionID = pSession->SessionID;
	DWORD Index = SESSIONID_INDEX_LOW(pSession->SessionID);
	DWORD sessionID = SESSIONID_ID_HIGH(pSession->SessionID);
	pSession->SessionID = INVALID_SESSIONID;
	pSession->socket = INVALID_SOCKET;


	// SendQ의 SerialRingBuffer RefCount Sub 및 SendQ에서 지우기
	int UseSize = pSession->SendQ.GetUseSize();
	unsigned int DirectSize = pSession->SendQ.DirectDequeueSize();
	int SubCount = UseSize / 8;
	if (DirectSize / 8 >= SubCount)
	{
		for (unsigned int iCnt = 0; iCnt < SubCount; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(pSession->SendQ.front() + iCnt * 8));
			pPacket->SubRef();
		}
	}
	else
	{
		int iCnt_1 = DirectSize / 8;
		int iCnt_2 = SubCount - iCnt_1 - 1;
		if (iCnt_2 < 0)
			DebugBreak();

		for (int iCnt = 0; iCnt < iCnt_1; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(pSession->SendQ.front() + iCnt * 8));
			pPacket->SubRef();
		}
		char Buffer[8];

		memcpy(Buffer, pSession->SendQ.front() + iCnt_1 * 8, DirectSize - iCnt_1 * 8);
		memcpy(Buffer + DirectSize - iCnt_1 * 8, pSession->SendQ.buffer(), 8 - (DirectSize - iCnt_1 * 8));
		SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)Buffer);
		pPacket->SubRef();

		char* front = pSession->SendQ.buffer() + 8 - (DirectSize - iCnt_1 * 8);
		for (int iCnt = 0; iCnt < iCnt_2; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(front + iCnt * 8));
			pPacket->SubRef();
		}
	}

	EnterCriticalSection(&CS_SessionIndex);
	SessionIndex.push(Index);
	LeaveCriticalSection(&CS_SessionIndex);

	return SessionID;
}

void CNETWORK::OutputDebugStringF(LPCTSTR format, ...)
{
	WCHAR buffer[1000] = { 0 };

	va_list arg; // 가변인수 벡터

	va_start(arg, format);
	vswprintf_s(buffer, 1000, format, arg);
	va_end(arg);

	OutputDebugString(buffer);
	return;
}

void CNETWORK::Crash()
{
	// 이 코드는 null 포인터를 역참조하여 크래시를 발생시킵니다.
	int* Nint = NULL;
	*Nint = 0;
}

void CNETWORK::RegisterTimer(void (*TimerFunction)(LPVOID), LPVOID p, DWORD dwMilliseconds)
{
	if (InterlockedAdd(&TimerCount, 0) == MAXIMUM_WAIT_OBJECTS)
	{
		LOG(L"Network", CLog::LEVEL_ERROR, L"TimerList Lack");
		Crash();
	}

	hTimers[TimerCount] = CreateWaitableTimer(NULL, FALSE, NULL);
	if (hTimers[TimerCount] == NULL)
	{
		int err = GetLastError();
		LOG(L"Network", CLog::LEVEL_ERROR, L"Failed CreateWaitableTimer() ErrorCode : %d", err);
		Crash();
	}

	TimerList[TimerCount].TimerFunction = TimerFunction;
	TimerList[TimerCount].Time.QuadPart = (dwMilliseconds * -10000LL);
	TimerList[TimerCount].p = p;


	InterlockedIncrement(&TimerCount);
	return;
}

unsigned WINAPI CNETWORK::WorkerThreadWrapping(LPVOID p)
{
	CNETWORK* This = (CNETWORK*)p;
	This->WorkerThread();

	return 0;
}
void CNETWORK::WorkerThread()
{
	bool retval;
	DWORD ThreadID = GetCurrentThreadId();

	Session* pSession;
	DWORD cbTransferred;
	OVERLAPPED* Overlapped;

	while (1)
	{
		cbTransferred = 0;
		pSession = nullptr;

		// 비동기 입출력(I/O) 대기
		retval = GetQueuedCompletionStatus(hCP, &cbTransferred,
			(PULONG_PTR)&pSession, (LPOVERLAPPED*)&Overlapped, INFINITE);

		// GQCS 오류로 인한 실패
		if (Overlapped == nullptr && retval == false)
		{
			int Err = GetLastError();
			LOG(L"Network", CLog::LEVEL_SYSTEM, L"Failed GQCS ErrorCode : %d", Err);
			PostQueuedCompletionStatus(hCP, 0, 0, 0);
			break;
		}

		// PQCS 구분 (cbTransferred는 항상 0으로 두고 pSession, Ovelapped를 바꾼다)
		if (retval == true && cbTransferred == 0)
		{
			// WorkerThread 종료
			if (Overlapped == (OVERLAPPED*)0)
			{
				PostQueuedCompletionStatus(hCP, 0, 0, 0);
				break;
			}

			// SendPost
			if (Overlapped == (OVERLAPPED*)1)
			{
				// WSASend
				SendPost(pSession);

				if (0 == InterlockedDecrement(&pSession->IOCount))
					SessionRelease(pSession);

				continue;
			}

			// 예상치 못한 PQCS가 왔을 경우
			LOG(L"Network", CLog::LEVEL_ERROR,
				L"Unexpected PQCS [retGQCS = %d] [cbTransferred = %ld] [pSession = %lld] [Overlapped = %lld]"
				, retval, cbTransferred, pSession, Overlapped);
			Crash();
		}

		// 세션 종료 : 클라에서 RST 보낸 경우 (실패한 I/O에 대한 완료통지를 처리합니다.)
		if (cbTransferred == 0)
		{
			int Err = GetLastError();
			NetworkError(Err, L"Asynchronous I/O");
			//OutputDebugStringF(L"[ret : %d] [Transferred : %d] [Err : %d] [SessionID : %lld]\n", retval, cbTransferred, Err, pSession->SessionID);

			Disconnect(pSession);

			// 실패한 완료통지에 대한 IOCount 감소시킵니다.
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			continue;
		}

		// WSARecv 완료   
		if (Overlapped == &pSession->RecvOverlapped)
		{
			// RecvQ rear 이동
			pSession->RecvQ.MoveRear(cbTransferred);

			// 메시지 처리
			NetworkMessageProc(pSession);

			// Recv 다시 걸어 둡니다.
			RecvPost(pSession);
			continue;
		}

		// WSASend 완료
		if (Overlapped == &pSession->SendOverlapped)
		{
			// SendQ의 SerialRingBuffer RefCount Sub 및 SendQ에서 지우기
			int UseSize = pSession->SendQ.GetUseSize();
			unsigned int DirectSize = pSession->SendQ.DirectDequeueSize();
			if (DirectSize / 8 >= pSession->SendBufferCount)
			{
				for (unsigned int iCnt = 0; iCnt < pSession->SendBufferCount; ++iCnt)
				{
					SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(pSession->SendQ.front() + iCnt * 8));
					pPacket->SubRef();
				}
			}
			else
			{
				int iCnt_1 = DirectSize / 8;
				int iCnt_2 = pSession->SendBufferCount - iCnt_1 - 1;
				if (iCnt_2 < 0)
					DebugBreak();

				for (int iCnt = 0; iCnt < iCnt_1; ++iCnt)
				{
					SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(pSession->SendQ.front() + iCnt * 8));
					pPacket->SubRef();
				}
				char Buffer[8];

				memcpy(Buffer, pSession->SendQ.front() + iCnt_1 * 8, DirectSize - iCnt_1 * 8);
				memcpy(Buffer + DirectSize - iCnt_1 * 8, pSession->SendQ.buffer(), 8 - (DirectSize - iCnt_1 * 8));
				SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)Buffer);
				pPacket->SubRef();

				char* front = pSession->SendQ.buffer() + 8 - (DirectSize - iCnt_1 * 8);
				for (int iCnt = 0; iCnt < iCnt_2; ++iCnt)
				{
					SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(front + iCnt * 8));
					pPacket->SubRef();
				}
			}

			pSession->SendQ.MoveFront(pSession->SendBufferCount * 8);
			pSession->SendBufferCount = 0;

			// SendFlag 해제
			InterlockedExchange(&pSession->SendFlag, 0);
			if (0 != pSession->SendQ.GetUseSize())
				SendPost(pSession);

			// 성공한 완료통지에 대한 IOCount 감소시킵니다.
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
		}
	}

	return;
}

unsigned WINAPI CNETWORK::TimerThreadWrapping(LPVOID p)
{
	CNETWORK* This = (CNETWORK*)p;
	This->TimerThread();

	return 0;
}
void CNETWORK::TimerThread()
{
	for (int iCnt = 0; iCnt < TimerCount; ++iCnt)
	{
		if (0 == SetWaitableTimer(hTimers[iCnt], &TimerList[iCnt].Time, 0, NULL, NULL, 0))
		{
			int err = GetLastError();
			LOG(L"Network", CLog::LEVEL_ERROR, L"Failed SetWaitableTimer() ErrorCode : %d", err);
			Crash();
		}
	}

	if (TimerCount == 0)
		return;

	while (1)
	{
		DWORD Index = WaitForMultipleObjects(TimerCount, hTimers, FALSE, INFINITE);
		TimerList[Index].TimerFunction(TimerList[Index].p);
		if (0 == SetWaitableTimer(hTimers[Index], &TimerList[Index].Time, 0, NULL, NULL, 0))
		{
			int err = GetLastError();
			LOG(L"Network", CLog::LEVEL_ERROR, L"Failed SetWaitableTimer() ErrorCode : %d", err);
			Crash();
		}
	}

	return;
}

unsigned WINAPI CNETWORK::AcceptThreadWrapping(LPVOID p)
{
	CNETWORK* This = (CNETWORK*)p;
	This->AcceptThread();

	return 0;
}
void CNETWORK::AcceptThread()
{
	static DWORD SessionID = 1;
	DWORD ThreadID = GetCurrentThreadId();

	SOCKET ClientSocket;
	SOCKADDR_IN ClientAddr;
	int AddrLen = sizeof(ClientAddr);

	while (1)
	{
		ZeroMemory(&ClientSocket, sizeof(ClientSocket));

		// accept()
		ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &AddrLen);
		if (ClientSocket == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			if (err == WSAEINTR)	// accept 블락 중에 ListenSocket이 CloseHandle 할 때 나오는 에러(AccpetThread 종료할 때 사용)
				break;
			else
			{
				LOG(L"Network", CLog::LEVEL_ERROR, L"Fail accept() ErrorCode : %d", err);
				continue;
			}
		}
		InterlockedIncrement(&Pnd.AcceptTotalCount);

		// IP와 port 세팅
		WCHAR IP[16] = { 0, };
		WORD Port;
		Port = ntohs(ClientAddr.sin_port);
		InetNtop(AF_INET, &ClientAddr.sin_addr, IP, 16);
		if (OnAcceptRequest(IP, Port) == false)
			continue;


		// 비동기 I/O를 위한 TCP SendBufSize = 0
		int optval = 0;
		LINGER lingeropt;
		lingeropt.l_linger = 0;
		lingeropt.l_onoff = 1;
		setsockopt(ClientSocket, SOL_SOCKET, SO_LINGER, (char*)&lingeropt, sizeof(lingeropt));
		//setsockopt(ClientSocket, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));

		// 세션 구조체 할당 및 초기화
		Session* pSession = SessionAlloc(ClientSocket, SessionID++);
		OnSessionConnected(pSession->SessionID);

		// 소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)ClientSocket, hCP, (ULONG_PTR)pSession, 0);

		// 비동기 입출력 시작
		RecvPost(pSession);

		// Monitor용
		InterlockedIncrement(&Pnd.AcceptTPS);
	}

	// AcceptThread 종료
	LOG(L"Network", CLog::LEVEL_SYSTEM, L"AcceptThread Release");

	return;
}