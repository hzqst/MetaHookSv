#include <metahook.h>

cl_enginefunc_t gEngfuncs;

WNDPROC g_MainWndProc;

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_COPYDATA:
		{
			COPYDATASTRUCT *pCopyData = (COPYDATASTRUCT*)lParam;

			char command[128];
			strcpy(command, (char *)pCopyData->lpData);
			command[pCopyData->cbData] = 0;

			gEngfuncs.pfnClientCmd(command);
			return 1;
		}
	}

	return CallWindowProc(g_MainWndProc, hWnd, message, wParam, lParam);
}

int Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion)
{
	memcpy(&gEngfuncs, pEnginefuncs, sizeof(gEngfuncs));

	HWND hWnd = FindWindow("Valve001", NULL);
	g_MainWndProc = (WNDPROC)GetWindowLong(hWnd, GWL_WNDPROC);
	SetWindowLong(hWnd, GWL_WNDPROC, (LONG)MainWndProc);

	return gExportfuncs.Initialize(pEnginefuncs, iVersion);
}