#pragma section(".blob", read,write)
__declspec(allocate(".blob"))
_declspec(dllexport) unsigned char g_pBlobBuffer[0x3000000];