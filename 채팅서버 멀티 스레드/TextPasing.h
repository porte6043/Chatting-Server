#ifndef __TEXTPARSER__
#define __TEXTPARSER__

int GetFileSize(const char* filename);

void GetLoadData(char* pbuffer, int size, const char* filename);

bool GetValueChar(char* pbuffer, char* pvalue, const char* findword, int filesize = 0);

bool GetValueInt(char* pbuffer, int* pvalue, const char* findword, int filesize);

void SkinNone(char* pbuffer);

#endif