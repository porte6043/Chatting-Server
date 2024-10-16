#include <conio.h>

#include "ChattingServer.h"
#include "공용 라이브러리/CrushDump.h"

CCrushDump CrushDump;

int main()
{
	ChattingServer Server;
	Server.Start(
		Server.IP,
		Server.Port,
		Server.WorkerThread,
		Server.ActiveThread,
		Server.Nagle,
		Server.ZeroCopySend,
		Server.PacketCode,
		Server.PacketKey,
		Server.SessionMax
	);

	while (1)
	{

		Sleep(999);

		// 키보드
		if (_kbhit())
		{
			// 눌린 값 대입
			switch (_getch())
			{
			case 'p':
				break;
			}
		}

		// 모니터링
		Server.monitor();
	}
	
	return 0;
}