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
	// ��ü ���� ũ�� ���
	int GetBufferSize();

	// ���� ���ۿ� ������� �뷮 ���
	int GetUseSize();

	// ���� ���ۿ� ��� ���ɷ� ���
	int GetFreeSize();

	// ��ȯ�� : ������ enqueue �� ������
	int enqueue(const void* pData, int Size);

	// ��ȯ�� : ������ dequeue �� ������
	int dequeue(void* pDest, int Size);

	int peek(void* chpdest, int chpdestsize);

	// ������ ��� ������ ����
	void clear();

	// buffer�� header ������ ����
	char* head();

	// buffer�� front ������ ����
	char* front();

	// buffer�� rear ������ ����
	char* rear();

	// buffer�� ������ ����
	char* buffer();

	// size��ŭ front �̵�
	void MoveFront(int size);

	// size��ŭ rear �̵�
	void MoveRear(int size);

	// Rear �����ͷ� ���������� �̵��� �� �ִ� ������
	int DirectEnqueueSize();

	// Front �����ͷ� ���������� �̵��� �� �ִ� ������
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

	CPacket();									// �⺻ ������
	CPacket(CPacket& other);					// Reference ������ -> �⺻�Լ����ڷ� ����
	CPacket(const CPacket& other);				// ���� ������
	CPacket& operator=(const CPacket& other);	// ���� ���� ������
	~CPacket();									// �Ҹ���
public:
	CPacket(SerialRingBuffer* packet);			// �ʱ�ȭ ������ : Test t1(t2); , Test t3 = t2;
	
public:
	static CPacket& Alloc();
	void copy(CPacket& Src);
	int GetUseSize();
	void clear();

	// ������ ����
	int PutData(const void* pSrc, int Size);
	CPacket& operator<<(BYTE& chValue);
	CPacket& operator<<(WORD& shValue);
	CPacket& operator<<(INT64& llValue);
	CPacket& operator<<(void* llValue);
	 

	// ������ ������
	int GetData(void* pDest, int Size);
	CPacket& operator>>(WORD& shValue);
	CPacket& operator>>(INT64& llValue);
	CPacket& operator>>(void* llValue);
};
#endif