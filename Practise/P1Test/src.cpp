#include <windows.h>
#include <stdio.h>
#define SYMBLICOBJ L"\\\\.\\LH_TESTOBJ"  // Ӧ�ò���ʷ�������ʱ,·������\\.\�µģ�ת��һ��\\\\.\\

int main()
{
	HANDLE kernelObj = CreateFile(SYMBLICOBJ, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, NULL);
	if (kernelObj == INVALID_HANDLE_VALUE)
	{
		printf("Open Device Failed:%d\n",GetLastError());
		system("pause");
		return 0;
	}
	char buf[5] = "1234";
	DWORD num = 0;
	if (!WriteFile(kernelObj, buf, 5, &num, NULL))
	{
		perror("Write ERROR");
	}
	else
	{
		printf("write success");
	}
	system("pause");
}