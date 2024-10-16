#ifndef __CTLSPOOL__
#define __CTLSPOOL__

#include <iostream>
#include <Windows.h>


template <typename DATA>
class CTlsPool
{
public:
	struct NODE
	{
		DATA _Data;
		NODE* _Next;
		NODE* _RootNext;

		NODE() : _Next(nullptr), _RootNext(nullptr) {}
	};

	struct TLSPOOL
	{
		NODE* Top;
		NODE* Bottom;
		DWORD TopSize;
		//DWORD UseSize;		// 디버깅용
		//DWORD BucketCount;	// 디버깅용

		//TLSPOOL() : Top(nullptr), Bottom(nullptr), TopSize(0), UseSize(0), BucketCount(0) {}
		TLSPOOL() : Top(nullptr), Bottom(nullptr), TopSize(0) {}
	};

private:
	DWORD				_TlsIndex;
	NODE* _RootNode;
	DWORD				_BucketSize;
	DWORD				_BucketTotalCount;
	DWORD				_BucketUseCount;
	SRWLOCK				_SrwLock;
	TLSPOOL				_TlsPool[20];
	DWORD				_PoolIndex;

private:
	NODE* BucketAlloc();

	void BucketFree();

	void BucketPush(NODE* node);

	NODE* BucketPop();

	NODE* RootPoolAlloc();

	void RootPoolFree(NODE* node);

	void NodePush(NODE* node);

	NODE* NodePop();

	void Crash();



public:
	CTlsPool(int bucketsize, int bucketcount);

	CTlsPool();

	~CTlsPool();

	void TlsPoolInit(int bucketsize, int bucketcount);

	DATA* Alloc();

	void Free(DATA* pData);

	void Monitor();

	int GetTotalCount() { return _BucketSize * _BucketTotalCount; }

	int GetUseCount() { return _BucketSize * _BucketUseCount; }

};

template <typename DATA>
void CTlsPool<DATA>::Monitor()
{
	printf("--------------------------------------------------\n");
	printf("RootPool BucketTotalCount	: %d\n", _BucketTotalCount * _BucketSize);
	printf("RootPool BucketUseCount		: %d\n\n", _BucketUseCount * _BucketSize);

	for (int iCnt = 0; iCnt < 3; ++iCnt)
	{
		int TopSize = _TlsPool[iCnt].TopSize;
		if (_TlsPool[iCnt].Bottom != nullptr)
			TopSize = _BucketSize;
		printf("Thread_%d TopSize		: %d\n", iCnt, TopSize);
		printf("Thread_%d UseSize		: %d\n", iCnt, _TlsPool[iCnt].UseSize);
		printf("Thread_%d BucketCount		: %d\n\n", iCnt, _TlsPool[iCnt].BucketCount);
	}

	printf("\n\n");
}

template <typename DATA>
CTlsPool<DATA>::CTlsPool(int bucketsize, int bucketcount)
	: _RootNode(nullptr), _BucketSize(bucketsize), _BucketTotalCount(0), _BucketUseCount(bucketcount), _PoolIndex(-1)
{
	_TlsIndex = TlsAlloc();
	if (_TlsIndex == TLS_OUT_OF_INDEXES)
		Crash();

	InitializeSRWLock(&_SrwLock);

	for (int iCnt = 0; iCnt < bucketcount; ++iCnt)
	{
		NODE* Bucket = BucketAlloc();
		BucketPush(Bucket);
	}
}

template <typename DATA>
CTlsPool<DATA>::CTlsPool()
	: _RootNode(nullptr), _BucketSize(0), _BucketTotalCount(0), _BucketUseCount(0), _PoolIndex(-1)
{
	_TlsIndex = TlsAlloc();
	if (_TlsIndex == TLS_OUT_OF_INDEXES)
		Crash();

	InitializeSRWLock(&_SrwLock);
}

// 여기 구현해야함
template <typename DATA>
CTlsPool<DATA>::~CTlsPool()
{
	/*for (int iCnt = 0; iCnt < _BucketTotalCount; ++iCnt)
	{
		BucketFree();
	}*/
}

template <typename DATA>
void CTlsPool<DATA>::TlsPoolInit(int bucketsize, int bucketcount)
{
	_BucketSize = bucketsize;
	_BucketUseCount = bucketcount;

	for (int iCnt = 0; iCnt < bucketcount; ++iCnt)
	{
		NODE* Bucket = BucketAlloc();
		BucketPush(Bucket);
	}
}


template <typename DATA>
typename CTlsPool<DATA>::NODE* CTlsPool<DATA>::BucketAlloc()
{
	NODE* BucketNode = nullptr;
	for (unsigned int iCnt = 0; iCnt < _BucketSize; ++iCnt)
	{
		NODE* NewNode = new NODE;
		NewNode->_Next = BucketNode;
		BucketNode = NewNode;
	}

	InterlockedIncrement(&_BucketTotalCount);

	return BucketNode;
}

template <typename DATA>
void CTlsPool<DATA>::BucketFree()
{
	NODE* DeleteNode = _RootNode;
	NODE* DeleteNextNode = nullptr;
	_RootNode = DeleteNode->_RootNext;

	for (int iCnt = 0; iCnt < _BucketSize; ++iCnt)
	{
		DeleteNextNode = DeleteNode->_Next;
		delete DeleteNode;
	}

	--_BucketTotalCount;

	return;
}

template <typename DATA>
void CTlsPool<DATA>::BucketPush(NODE* node)
{
	AcquireSRWLockExclusive(&_SrwLock);
	node->_RootNext = _RootNode;
	_RootNode = node;
	--_BucketUseCount;
	ReleaseSRWLockExclusive(&_SrwLock);

	return;
}

template <typename DATA>
typename CTlsPool<DATA>::NODE* CTlsPool<DATA>::BucketPop()
{
	AcquireSRWLockExclusive(&_SrwLock);
	if (_RootNode == nullptr)
		_RootNode = BucketAlloc();

	NODE* Node = _RootNode;
	_RootNode = _RootNode->_RootNext;
	++_BucketUseCount;
	ReleaseSRWLockExclusive(&_SrwLock);

	return Node;
}

template <typename DATA>
typename CTlsPool<DATA>::NODE* CTlsPool<DATA>::RootPoolAlloc()
{
	NODE* Node;
	if (_RootNode == nullptr)
	{
		Node = BucketAlloc();
		AcquireSRWLockExclusive(&_SrwLock);
		++_BucketUseCount;
		ReleaseSRWLockExclusive(&_SrwLock);

		return Node;
	}

	Node = BucketPop();
	return Node;
}

template <typename DATA>
void CTlsPool<DATA>::RootPoolFree(NODE* node)
{
	BucketPush(node);
	return;
}

template <typename DATA>
void CTlsPool<DATA>::NodePush(NODE* node)
{
	TLSPOOL* TlsPool = (TLSPOOL*)TlsGetValue(_TlsIndex);
	if (TlsPool == NULL)
	{
		DWORD PoolIndex = InterlockedIncrement(&_PoolIndex);
		TlsSetValue(_TlsIndex, &_TlsPool[PoolIndex]);
		TlsPool = &_TlsPool[PoolIndex];
	}

	if (TlsPool->TopSize == _BucketSize)
	{
		if (TlsPool->Bottom == nullptr)
		{
			TlsPool->Bottom = TlsPool->Top;
			TlsPool->Top = nullptr;
			TlsPool->TopSize = 0;
		}
		else
		{
			RootPoolFree(TlsPool->Top);
			TlsPool->Top = nullptr;
			TlsPool->TopSize = 0;
		}
	}

	node->_Next = TlsPool->Top;
	TlsPool->Top = node;
	++TlsPool->TopSize;

	return;
}

template <typename DATA>
typename CTlsPool<DATA>::NODE* CTlsPool<DATA>::NodePop()
{
	TLSPOOL* TlsPool = (TLSPOOL*)TlsGetValue(_TlsIndex);
	if (TlsPool == NULL)
	{
		DWORD PoolIndex = InterlockedIncrement(&_PoolIndex);
		TlsSetValue(_TlsIndex, &_TlsPool[PoolIndex]);
		TlsPool = &_TlsPool[PoolIndex];
	}

	if (TlsPool->Top == nullptr)
	{
		if (TlsPool->Bottom == nullptr)
		{
			TlsPool->Top = RootPoolAlloc();
			TlsPool->TopSize = _BucketSize;
		}
		else
		{
			TlsPool->Top = TlsPool->Bottom;
			TlsPool->TopSize = _BucketSize;
			TlsPool->Bottom = nullptr;
		}
	}

	NODE* Node = TlsPool->Top;
	TlsPool->Top = Node->_Next;
	--TlsPool->TopSize;

	return Node;
}

template <typename DATA>
typename DATA* CTlsPool<DATA>::Alloc()
{
	return (DATA*)NodePop();
}

template <typename DATA>
void CTlsPool<DATA>::Free(DATA* pData)
{
	NodePush((NODE*)pData);
	return;
}

template <typename DATA>
void CTlsPool<DATA>::Crash()
{
	// 이 코드는 null 포인터를 역참조하여 크래시를 발생시킵니다.
	int* ptr = nullptr;
	*ptr = 10;
}
#endif