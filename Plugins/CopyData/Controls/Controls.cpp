#define _WIN32_WINNT 0x500
#include <windows.h>
#include <conio.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	if (argc < 2)
		return 0;

	char *command = argv[1];
	HWND hWnd = FindWindow("Valve001", NULL);

	if (!hWnd)
		return 0;

	COPYDATASTRUCT CopyData;
	CopyData.lpData = command;
	CopyData.cbData = strlen(command);
	SendMessage(hWnd, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&CopyData);
	return 1;
}