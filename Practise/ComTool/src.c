#include <windows.h>
#include <stdio.h>
//���ڵ��Դ��ڵ�С���ߣ�ʵ�ֶԴ��ڽ��ж�д�����С����
//�趨��һ�������в�������ָ�����ڵ�����
int main(int argc,char** argv)
{

	HANDLE handle=CreateFile("COM1",				//��������
		GENERIC_READ|GENERIC_WRITE,	//Ȩ��
		0,
		NULL,OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,NULL
		);
	if (handle == INVALID_HANDLE_VALUE)
	{
		printf("Open Com %s Failed ,Error:%d\n", "COM1", GetLastError());
		return 0;
	}
	if(SetupComm(handle, 1024, 1024) == 0)
	{
		printf("Setup Com %s Failed ,Error:%d\n", "COM1", GetLastError());
		return 0;
	}
	char* msg1 = "I Write a msg\n";
	int len = strlen(msg1) + 1;
	char buf[10] = { 0 };
	int num = 0;
	OVERLAPPED op = { 0 };

	OVERLAPPED op2 = { 0 };
	while (TRUE)
	{
		Sleep(2000);
		op.Offset = 0xffffffff;
		op.OffsetHigh = 0xffffffff;
		if (FALSE == WriteFile(handle, msg1, len, NULL, &op))
		{
			if (GetLastError() != ERROR_IO_PENDING)
			{
				printf("Write F err:%d\n", GetLastError());
			}
			else
			{
				printf("Write Pending\n");
				if (WaitForSingleObject(op.hEvent, INFINITE) == 0)
				{
					printf("Write COMPLETE\n");
				}
			}

		}

		op.Offset = 0;
		op.OffsetHigh = 0;
		if (FALSE == ReadFile(handle, buf, 10, NULL, &op2))
		{
			if (GetLastError() != ERROR_IO_PENDING)
			{
				printf("Read F err:%d\n", GetLastError());
			}
			printf("Read Pending\n");
			if (WaitForSingleObject(op2.hEvent, INFINITE) == 0)
			{
				printf("READ COMPLETE\n");
			}

		}

	}

}