#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "TextPasing.h"


int g_iCt; // �˻� ���� �ε��� => SkipNone �Լ� ������ �ε����� ������ �־���
int g_FileSize; // ���� ������


//---------------------------------------------------------------
// ������ ��� ���� ������ ��ŭ �����Ҵ�
//---------------------------------------------------------------
int GetFileData(char** pbuffer, const char* filename)
{
	g_iCt = 0;
	int FileSize;
	FILE* pFile;

	if (fopen_s(&pFile, filename, "r") != 0) return false; // r�� ���͸� \n���� ���� , rb�� ���͸� \r\n ���� ����
	fseek(pFile, 0, SEEK_END);
	g_FileSize = ftell(pFile);
	FileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	*pbuffer = (char*)malloc(g_FileSize);
	fread(*pbuffer, 1, g_FileSize, pFile);
	fclose(pFile);

	return FileSize;
}

//---------------------------------------------------------------
// �����Ͱ� ���ڿ��϶�
//---------------------------------------------------------------
bool GetValueChar(char* pbuffer, char** pvalue, const char* findword, int filesize)
{
	if (filesize != 0)
	{
		g_FileSize = filesize;
	}

	char chWord[256];
	size_t WordLen = strlen(findword);
	g_iCt = 0;
	int DataLen = 0;
	*pvalue = NULL;

	// chWord 0���� �ʱ�ȭ
	memset(chWord, 0, 256);


	for (g_iCt; g_iCt < g_FileSize; ++g_iCt)
	{
		SkinNone(pbuffer);

		// Word ã��
		if (findword[0] == pbuffer[g_iCt])
		{
			memcpy(chWord, &pbuffer[g_iCt], WordLen); // ���� '\0' �� �Ⱥٿ��൵ �ǳ�? => ������ memset���� 0���� �ʱ�ȭ �Ͽ��� ���� ��� ����
			if (0 == strcmp(findword, chWord))
			{
				g_iCt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
				//---------------------------------------------------------
				if (pbuffer[g_iCt] != ' ' && pbuffer[g_iCt] != '\t' && pbuffer[g_iCt] != '\n' && pbuffer[g_iCt] != '/')
				{
					g_iCt -= WordLen;
					continue;
				}
				SkinNone(pbuffer);

				if (pbuffer[g_iCt] == '=')
				{
					++g_iCt;
					SkinNone(pbuffer);

					if (pbuffer[g_iCt] == '\"')
					{
						while (1)
						{
							++g_iCt;
							if (pbuffer[g_iCt] == '\"')
								break;
							++DataLen;
						}

						*pvalue = (char*)malloc(DataLen + 1);
						memset(*pvalue, 0, DataLen + 1);
						memcpy(*pvalue, &pbuffer[g_iCt - DataLen], DataLen);

						return true;
					}

					if (pbuffer[g_iCt] == '\'')
					{
						while (1)
						{
							++g_iCt;
							if (pbuffer[g_iCt] == '\'')
								break;
							++DataLen;
						}

						*pvalue = (char*)malloc(DataLen);
						memset(*pvalue, 0, DataLen);
						memcpy(*pvalue, &pbuffer[g_iCt - DataLen], DataLen);

						return true;
					}

					return false;
				}
				return false;
			}
		}
	}
	return false;
}

//---------------------------------------------------------------
// �����Ͱ� �����϶�
//---------------------------------------------------------------
bool GetValueInt(char* pbuffer, int* pvalue, const char* findword)
{
	char chWord[256];
	size_t WordLen = strlen(findword);
	g_iCt = 0;
	int DataLen = 0;

	// chWord 0���� �ʱ�ȭ
	memset(chWord, 0, 256);


	for (g_iCt; g_iCt < g_FileSize; ++g_iCt)
	{
		SkinNone(pbuffer);

		// Word ã��
		if (findword[0] == pbuffer[g_iCt])
		{
			memcpy(chWord, &pbuffer[g_iCt], WordLen); // ���� '\0' �� �Ⱥٿ��൵ �ǳ�? => ������ memset���� 0���� �ʱ�ȭ �Ͽ��� ���� ��� ����
			if (0 == strcmp(findword, chWord))
			{
				g_iCt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
				//---------------------------------------------------------
				if (pbuffer[g_iCt] != ' ' && pbuffer[g_iCt] != '\t' && pbuffer[g_iCt] != '\n' && pbuffer[g_iCt] != '/')
				{
					g_iCt -= WordLen;
					continue;
				}
				SkinNone(pbuffer);

				if (pbuffer[g_iCt] == '=')
				{
					++g_iCt;
					SkinNone(pbuffer);
					while (pbuffer[g_iCt] >= '0' && pbuffer[g_iCt] <= '9')
					{
						++g_iCt;
						++DataLen;
					}
					memset(chWord, 0, 256);
					memcpy(chWord, &pbuffer[g_iCt - DataLen], DataLen);
					*pvalue = atoi(chWord);

					return true;
				}
				return false;
			}
		}
	}
	return false;
}

//---------------------------------------------------------------
// �ּ�, ��� ���� ó��
//---------------------------------------------------------------
void SkinNone(char* pbuffer)
{
	// ����(' ') �ѱ��
	while (pbuffer[g_iCt] == ' ' || pbuffer[g_iCt] == '\t' || pbuffer[g_iCt] == '\n')
	{
		++g_iCt;
	}


	//  �ּ� �ѱ��(//)
	if (pbuffer[g_iCt] == '/' && pbuffer[g_iCt + 1] == '/') // ASCII : 0x0A = \n
	{
		while (1)
		{
			++g_iCt;
			if (pbuffer[g_iCt] == '\n')
				if (pbuffer[g_iCt + 1] == '/')
				{
					--g_iCt;
					break;
				}
				else
				{
					break;
				}
		}
	}

	// �ּ� �ѱ��(/**/)
	if (pbuffer[g_iCt] == '/' && pbuffer[g_iCt + 1] == '*')
	{
		++g_iCt;
		while (1)
		{
			++g_iCt;
			if (pbuffer[g_iCt] == '*' && pbuffer[g_iCt + 1] == '/')
			{
				++g_iCt;
				break;
			}
		}
		++g_iCt;
	}

	// ����(' ') �ѱ��
	while (pbuffer[g_iCt] == ' ' || pbuffer[g_iCt] == '\t' || pbuffer[g_iCt] == '\n')
	{
		++g_iCt;
	}


}













//------------------------------------------------------------------------------------------------------------------------------
// �ǵ�� ���� �߰� �Լ�

//---------------------------------------------------------------
// ������ ��� ���� ������ ���
//---------------------------------------------------------------
int GetFileSize(const char* filename)
{
	FILE* pFile;
	int FileSize;
	if (fopen_s(&pFile, filename, "r") != 0) return false; // r�� ���͸� \n���� ���� , rb�� ���͸� \r\n ���� ����
	fseek(pFile, 0, SEEK_END);
	FileSize = ftell(pFile);
	fclose(pFile);

	return FileSize;
}

//---------------------------------------------------------------
// ���� ��� ������ ��ŭ Load
//---------------------------------------------------------------
void GetLoadData(char* pbuffer, int size, const char* filename)
{
	FILE* pFile;

	fopen_s(&pFile, filename, "r");
	fread(pbuffer, 1, size, pFile);
	fclose(pFile);

	return;
}


bool GetValueChar(char* pbuffer, char* pvalue, const char* findword, int filesize)
{
	if (filesize != 0)
	{
		g_FileSize = filesize;
	}

	char chWord[256];
	size_t WordLen = strlen(findword);
	g_iCt = 0;
	int DataLen = 0;

	// chWord 0���� �ʱ�ȭ
	memset(chWord, 0, 256);


	for (g_iCt; g_iCt < g_FileSize; ++g_iCt)
	{
		SkinNone(pbuffer);

		// Word ã��
		if (findword[0] == pbuffer[g_iCt])
		{
			memcpy(chWord, &pbuffer[g_iCt], WordLen); // ���� '\0' �� �Ⱥٿ��൵ �ǳ�? => ������ memset���� 0���� �ʱ�ȭ �Ͽ��� ���� ��� ����
			if (0 == strcmp(findword, chWord))
			{
				g_iCt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
				//---------------------------------------------------------
				if (pbuffer[g_iCt] != ' ' && pbuffer[g_iCt] != '\t' && pbuffer[g_iCt] != '\n' && pbuffer[g_iCt] != '/')
				{
					g_iCt -= WordLen;
					continue;
				}
				SkinNone(pbuffer);

				if (pbuffer[g_iCt] == '=')
				{
					++g_iCt;
					SkinNone(pbuffer);

					if (pbuffer[g_iCt] == '\"')
					{
						while (1)
						{
							++g_iCt;
							if (pbuffer[g_iCt] == '\"')
								break;
							++DataLen;
						}

						memset(pvalue, 0, DataLen);
						memcpy(pvalue, &pbuffer[g_iCt - DataLen], DataLen);

						return true;
					}

					if (pbuffer[g_iCt] == '\'')
					{
						while (1)
						{
							++g_iCt;
							if (pbuffer[g_iCt] == '\'')
								break;
							++DataLen;
						}

						//memset(pvalue, 0, 256);
						memcpy(pvalue, &pbuffer[g_iCt - DataLen], DataLen);

						return true;
					}

					return false;
				}
				return false;
			}
		}
	}
	return false;
}

bool GetValueInt(char* pbuffer, int* pvalue, const char* findword, int filesize)
{
	char chWord[256];
	size_t WordLen = strlen(findword);
	g_iCt = 0;
	int DataLen = 0;
	g_FileSize = filesize;

	// chWord 0���� �ʱ�ȭ
	memset(chWord, 0, 256);


	for (g_iCt; g_iCt < g_FileSize; ++g_iCt)
	{
		SkinNone(pbuffer);

		// Word ã��
		if (findword[0] == pbuffer[g_iCt])
		{
			memcpy(chWord, &pbuffer[g_iCt], WordLen); // ���� '\0' �� �Ⱥٿ��൵ �ǳ�? => ������ memset���� 0���� �ʱ�ȭ �Ͽ��� ���� ��� ����
			if (0 == strcmp(findword, chWord))
			{
				g_iCt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
				//---------------------------------------------------------
				if (pbuffer[g_iCt] != ' ' && pbuffer[g_iCt] != '\t' && pbuffer[g_iCt] != '\n' && pbuffer[g_iCt] != '/')
				{
					g_iCt -= WordLen;
					continue;
				}
				SkinNone(pbuffer);

				if (pbuffer[g_iCt] == '=')
				{
					++g_iCt;
					SkinNone(pbuffer);
					while (pbuffer[g_iCt] >= '0' && pbuffer[g_iCt] <= '9')
					{
						++g_iCt;
						++DataLen;
					}
					memset(chWord, 0, 256);
					memcpy(chWord, &pbuffer[g_iCt - DataLen], DataLen);
					*pvalue = atoi(chWord);

					return true;
				}
				return false;
			}
		}
	}
	return false;
}


//------------------------------------------------------------------------------------------------------------------------------