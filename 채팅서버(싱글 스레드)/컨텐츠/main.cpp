#include <conio.h>

#include "ChattingServer.h"
#include "���� ���̺귯��/CrushDump.h"

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

		// Ű����
		if (_kbhit())
		{
			// ���� �� ����
			switch (_getch())
			{
			case 'p':
				break;
			}
		}

		// ����͸�
		Server.monitor();
	}
	
	return 0;
}