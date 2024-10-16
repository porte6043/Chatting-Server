#include <iostream>
#include <conio.h>

#include "CNetwork.h"
#include "ChattingServer2.h"
#include "CrushDump.h"
using namespace std;

CCrushDump Dump;

int main()
{
	ChattingServer2 Server;

	Server.Start(
		Server.IP,
		Server.Port,
		Server.WorkerThread,
		Server.ActiveThread,
		Server.Nagle,
		Server.SessionMax
	);


	bool ShutDown = false;
	while (!ShutDown)
	{

		Sleep(999);

		// 키보드
		if (_kbhit())
		{
			// 눌린 값 대입
			switch (_getch())
			{
			case 'd':
				// 서버 종료
				ShutDown = true;
				break;
			case 's':
				break;
			case 'p':
				CTlsProfiling::GetInstance()->ProfileDataOutText(L"Profile.txt");
				break;
			case 'u':
				break;
			}
		}
		// 모니터링
		Server.monitor();
	}


	return 0;
}