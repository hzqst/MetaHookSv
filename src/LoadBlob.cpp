#include <windows.h>
#include "LoadBlob.h"
#include <interface.h>
#include "IFileSystem.h"

extern IFileSystem_HL25* g_pFileSystem_HL25;
extern IFileSystem *g_pFileSystem;

#define FILESYSTEM_ANY_OPEN(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Open(__VA_ARGS__) : g_pFileSystem->Open(__VA_ARGS__))
#define FILESYSTEM_ANY_READ(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Read(__VA_ARGS__) : g_pFileSystem->Read(__VA_ARGS__))
#define FILESYSTEM_ANY_CLOSE(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Close(__VA_ARGS__) : g_pFileSystem->Close(__VA_ARGS__))
#define FILESYSTEM_ANY_SEEK(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Seek(__VA_ARGS__) : g_pFileSystem->Seek(__VA_ARGS__))
#define FILESYSTEM_ANY_TELL(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Tell(__VA_ARGS__) : g_pFileSystem->Tell(__VA_ARGS__))
#define FILESYSTEM_ANY_WRITE(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Write(__VA_ARGS__) : g_pFileSystem->Write(__VA_ARGS__))
#define FILESYSTEM_ANY_CREATEDIR(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->CreateDirHierarchy(__VA_ARGS__) : g_pFileSystem->CreateDirHierarchy(__VA_ARGS__))
#define FILESYSTEM_ANY_EOF(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->EndOfFile(__VA_ARGS__) : g_pFileSystem->EndOfFile(__VA_ARGS__))
#define FILESYSTEM_ANY_PARSEFILE(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->ParseFile(__VA_ARGS__) : g_pFileSystem->ParseFile(__VA_ARGS__))
#define FILESYSTEM_ANY_READLINE(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->ReadLine(__VA_ARGS__) : g_pFileSystem->ReadLine(__VA_ARGS__))
#define FILESYSTEM_ANY_ADDSEARCHPATH(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->AddSearchPath(__VA_ARGS__) : g_pFileSystem->AddSearchPath(__VA_ARGS__))
#define FILESYSTEM_ANY_MOUNT(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Mount(__VA_ARGS__) : g_pFileSystem->Mount(__VA_ARGS__))
#define FILESYSTEM_ANY_UNMOUNT(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Unmount(__VA_ARGS__) : g_pFileSystem->Unmount(__VA_ARGS__))

#ifndef _USRDLL
#pragma data_seg(".data")
BYTE g_pBlobBuffer[0x2000000];
#endif

BlobHeader_t g_BlobHeader;

BlobHeader_t *GetBlobHeader(void)
{
	return &g_BlobHeader;
}

BOOL FIsBlob(const char *pstFileName)
{
	FileHandle_t file = FILESYSTEM_ANY_OPEN(pstFileName, "rb");

	if (file == FILESYSTEM_INVALID_HANDLE)
		return FALSE;

	BlobInfo_t info;
	FILESYSTEM_ANY_READ(&info, sizeof(BlobInfo_t), file);
	FILESYSTEM_ANY_CLOSE(file);

	if (info.m_dwAlgorithm != BLOB_ALGORITHM)
		return FALSE;

	return TRUE;
}

DWORD NLoadBlobFile(const char *pstFileName, BlobFootprint_t *pblobfootprint, void **pv)
{
	FileHandle_t file = FILESYSTEM_ANY_OPEN(pstFileName, "rb");

	DWORD dwSize;
	BYTE *pBuffer;
	DWORD dwAddress;

	FILESYSTEM_ANY_SEEK(file, 0, FILESYSTEM_SEEK_TAIL);
	dwSize = FILESYSTEM_ANY_TELL(file);
	FILESYSTEM_ANY_SEEK(file, 0, FILESYSTEM_SEEK_HEAD);

	pBuffer = (BYTE *)malloc(dwSize);
	FILESYSTEM_ANY_READ(pBuffer, dwSize, file);

	dwAddress = LoadBlobFile(pBuffer, pblobfootprint, pv, dwSize);
	free(pBuffer);

	FILESYSTEM_ANY_CLOSE(file);

	return dwAddress;
}

DWORD LoadBlobFile(BYTE *pBuffer, BlobFootprint_t *pblobfootprint, void **pv, DWORD dwSize)
{
	BYTE bXor = 0x57;
	BlobHeader_t *pHeader;
	BlobSection_t *pSection;
	DWORD dwAddress = 0;

	for (size_t i = sizeof(BlobInfo_t); i < dwSize; i++)
	{
		pBuffer[i] ^= bXor;
		bXor += pBuffer[i] + 0x57;
	}

	pHeader = (BlobHeader_t *)(pBuffer + sizeof(BlobInfo_t));
	pHeader->m_dwExportPoint ^= 0x7A32BC85;
	pHeader->m_dwImageBase ^= 0x49C042D1;
	pHeader->m_dwEntryPoint -= 12;
	pHeader->m_dwImportTable ^= 0x872C3D47;
	pSection = (BlobSection_t *)(pBuffer + sizeof(BlobInfo_t) + sizeof(BlobHeader_t));

	memcpy(&g_BlobHeader, pHeader, sizeof(BlobHeader_t));

	for (WORD j = 0; j <= pHeader->m_wSectionCount; j++)
	{
		if (pSection[j].m_bIsSpecial)
			dwAddress = pSection[j].m_dwDataAddress;

		if (pSection[j].m_dwVirtualSize > pSection[j].m_dwDataSize)
			memset((BYTE *)(pSection[j].m_dwVirtualAddress + pSection[j].m_dwDataSize), NULL, pSection[j].m_dwVirtualSize - pSection[j].m_dwDataSize);

		memcpy((BYTE *)pSection[j].m_dwVirtualAddress, pBuffer + pSection[j].m_dwDataAddress, pSection[j].m_dwDataSize);
	}

	PIMAGE_IMPORT_DESCRIPTOR pImport = (PIMAGE_IMPORT_DESCRIPTOR)pHeader->m_dwImportTable;

	while (pImport->Name)
	{
		HMODULE hPorcDll = LoadLibrary((char *)(pHeader->m_dwImageBase + pImport->Name));
		PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)(pHeader->m_dwImageBase + pImport++->FirstThunk);

		while (pThunk->u1.Function)
		{
			const char *pszProcName = IMAGE_SNAP_BY_ORDINAL(pThunk->u1.Ordinal) ? (char *)((LONG)pThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG32 - 1) : (char *)(pHeader->m_dwImageBase + ((IMAGE_IMPORT_BY_NAME *)((LONG)pThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG32 - 1))->Name);
			pThunk++->u1.AddressOfData = (DWORD)GetProcAddress(hPorcDll, pszProcName);
		}
	}

	((BOOL (WINAPI *)(HINSTANCE, DWORD, void *))(pHeader->m_dwEntryPoint))(0, DLL_PROCESS_ATTACH, 0);
	((void (*)(void **))(pHeader->m_dwExportPoint))(pv);
	return dwAddress;
}

void FreeBlob(BlobFootprint_t *pblobfootprint)
{
	FreeLibrary(pblobfootprint->m_hDll);
}