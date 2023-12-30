#if defined(METAHOOK_BLOB_SUPPORT) || defined(_DEBUG)

#pragma bss_seg(".blob")
__declspec(allocate(".blob"))
unsigned char g_pBlobBuffer[0x3000000];

#endif