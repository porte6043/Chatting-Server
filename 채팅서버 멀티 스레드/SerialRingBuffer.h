#ifndef __SERIAL_RINGBUFFER__
#define __SERIAL_RINGBUFFER__
#include <Windows.h>
#include "CTlsPool.h"

#define df_PACKET_KEY  0x32
class CPacket;
class SerialRingBuffer;

enum TYPE
{
	basic,
	ref,
	copy,
	copymove,
	destory,
	init
};

struct Log
{
	DWORD ThreadID;
	TYPE Type;
	DWORD iocount;
};
struct LoggingPacket
{
	SerialRingBuffer* packet;
	Log log[3000];
	long index;
};


class SerialRingBuffer
{
public:
	enum Buffer
	{
		en_BUFFER_DEFAULT_SIZE = 6400
	};


private:
	char* Header;
	char* Buffer;
	char* Front;
	char* Rear;
	int HeaderSize;
	int BufferTotalSize;
	long HeaderFlag;
	long EncodeFlag;
	CRITICAL_SECTION CS_SerialRingBuffer;
	long RefCount;

public:
	SerialRingBuffer();
	//SerialRingBuffer(int headerSize, int bufferSize);

	~SerialRingBuffer();

public:
	// 전체 버퍼 크기 얻기
	int GetBufferSize();

	// 현재 버퍼에 사용중인 용량 얻기
	int GetUseSize();

	// 현재 버퍼에 사용 가능량 얻기
	int GetFreeSize();

	// 반환값 : 실제로 enqueue 된 사이즈
	int enqueue(const void* pData, int Size);

	// 반환값 : 실제로 dequeue 된 사이즈
	int dequeue(void* pDest, int Size);

	int peek(void* chpdest, int chpdestsize);

	// 버퍼의 모든 데이터 삭제
	void clear();

	// buffer의 header 포인터 얻음
	char* head();

	// buffer의 front 포인터 얻음
	char* front();

	// buffer의 rear 포인터 얻음
	char* rear();

	// buffer의 포인터 얻음
	char* buffer();

	// size만큼 front 이동
	void MoveFront(int size);

	// size만큼 rear 이동
	void MoveRear(int size);

	// Rear 포인터로 직접적으로 이동할 수 있는 사이즈
	int DirectEnqueueSize();

	// Front 포인터로 직접적으로 이동할 수 있는 사이즈
	int DirectDequeueSize();

	void encode();

	bool decode();

	// Header Set
	void SetHeader(const void* Src, int Size);

	void Free();

	static SerialRingBuffer* Alloc();

	static int GetPoolTotalCount();

	static int GetPoolUseCount();

	void AddRef();

	void SubRef();

	void Crash();

};



class CPacket 
{
public:
	SerialRingBuffer* Packet;

	CPacket();									// 기본 생성자
	CPacket(CPacket& other);					// Reference 생성자 -> 기본함수인자로 생성
	CPacket(const CPacket& other);				// 복사 생성자
	CPacket& operator=(const CPacket& other);	// 복사 대입 연산자
	~CPacket();									// 소멸자
public:
	CPacket(SerialRingBuffer* packet);			// 초기화 생성자 : Test t1(t2); , Test t3 = t2;
	
public:
	static CPacket& Alloc();
	void copy(CPacket& Src);
	int GetUseSize();
	void clear();

	// 데이터 삽입
	int PutData(const void* pSrc, int Size);
	CPacket& operator<<(BYTE& chValue);
	CPacket& operator<<(WORD& shValue);
	CPacket& operator<<(INT64& llValue);
	CPacket& operator<<(void* llValue);
	 

	// 데이터 꺼내기
	int GetData(void* pDest, int Size);
	CPacket& operator>>(WORD& shValue);
	CPacket& operator>>(INT64& llValue);
	CPacket& operator>>(void* llValue);
};
#endif