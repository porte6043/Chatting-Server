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

		// Ű����
		if (_kbhit())
		{
			// ���� �� ����
			switch (_getch())
			{
			case 'd':
				// ���� ����
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
		// ����͸�
		Server.monitor();
	}


	return 0;
}