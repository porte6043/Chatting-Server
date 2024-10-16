#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "TextPasing.h"


int g_iCt; // 검사 버퍼 인덱스 => SkipNone 함수 떄문에 인덱스를 전역에 넣었음
int g_FileSize; // 파일 사이즈


//---------------------------------------------------------------
// 파일을 열어서 파일 사이즈 만큼 동적할당
//---------------------------------------------------------------
int GetFileData(char** pbuffer, const char* filename)
{
	g_iCt = 0;
	int FileSize;
	FILE* pFile;

	if (fopen_s(&pFile, filename, "r") != 0) return false; // r은 엔터를 \n으로 읽음 , rb는 엔터를 \r\n 으로 읽음
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
// 데이터가 문자열일때
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

	// chWord 0으로 초기화
	memset(chWord, 0, 256);


	for (g_iCt; g_iCt < g_FileSize; ++g_iCt)
	{
		SkinNone(pbuffer);

		// Word 찾기
		if (findword[0] == pbuffer[g_iCt])
		{
			memcpy(chWord, &pbuffer[g_iCt], WordLen); // 끝에 '\0' 을 안붙여줘도 되나? => 위에서 memset으로 0으로 초기화 하였기 떄문 상관 없음
			if (0 == strcmp(findword, chWord))
			{
				g_iCt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
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
// 데이터가 정수일때
//---------------------------------------------------------------
bool GetValueInt(char* pbuffer, int* pvalue, const char* findword)
{
	char chWord[256];
	size_t WordLen = strlen(findword);
	g_iCt = 0;
	int DataLen = 0;

	// chWord 0으로 초기화
	memset(chWord, 0, 256);


	for (g_iCt; g_iCt < g_FileSize; ++g_iCt)
	{
		SkinNone(pbuffer);

		// Word 찾기
		if (findword[0] == pbuffer[g_iCt])
		{
			memcpy(chWord, &pbuffer[g_iCt], WordLen); // 끝에 '\0' 을 안붙여줘도 되나? => 위에서 memset으로 0으로 초기화 하였기 떄문 상관 없음
			if (0 == strcmp(findword, chWord))
			{
				g_iCt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
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
// 주석, 띄어 쓰기 처리
//---------------------------------------------------------------
void SkinNone(char* pbuffer)
{
	// 띄어쓰기(' ') 넘기기
	while (pbuffer[g_iCt] == ' ' || pbuffer[g_iCt] == '\t' || pbuffer[g_iCt] == '\n')
	{
		++g_iCt;
	}


	//  주석 넘기기(//)
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

	// 주석 넘기기(/**/)
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

	// 띄어쓰기(' ') 넘기기
	while (pbuffer[g_iCt] == ' ' || pbuffer[g_iCt] == '\t' || pbuffer[g_iCt] == '\n')
	{
		++g_iCt;
	}


}













//------------------------------------------------------------------------------------------------------------------------------
// 피드백 이후 추가 함수

//---------------------------------------------------------------
// 파일을 열어서 파일 사이즈 계산
//---------------------------------------------------------------
int GetFileSize(const char* filename)
{
	FILE* pFile;
	int FileSize;
	if (fopen_s(&pFile, filename, "r") != 0) return false; // r은 엔터를 \n으로 읽음 , rb는 엔터를 \r\n 으로 읽음
	fseek(pFile, 0, SEEK_END);
	FileSize = ftell(pFile);
	fclose(pFile);

	return FileSize;
}

//---------------------------------------------------------------
// 파일 열어서 사이즈 만큼 Load
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

	// chWord 0으로 초기화
	memset(chWord, 0, 256);


	for (g_iCt; g_iCt < g_FileSize; ++g_iCt)
	{
		SkinNone(pbuffer);

		// Word 찾기
		if (findword[0] == pbuffer[g_iCt])
		{
			memcpy(chWord, &pbuffer[g_iCt], WordLen); // 끝에 '\0' 을 안붙여줘도 되나? => 위에서 memset으로 0으로 초기화 하였기 떄문 상관 없음
			if (0 == strcmp(findword, chWord))
			{
				g_iCt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
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

	// chWord 0으로 초기화
	memset(chWord, 0, 256);


	for (g_iCt; g_iCt < g_FileSize; ++g_iCt)
	{
		SkinNone(pbuffer);

		// Word 찾기
		if (findword[0] == pbuffer[g_iCt])
		{
			memcpy(chWord, &pbuffer[g_iCt], WordLen); // 끝에 '\0' 을 안붙여줘도 되나? => 위에서 memset으로 0으로 초기화 하였기 떄문 상관 없음
			if (0 == strcmp(findword, chWord))
			{
				g_iCt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
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