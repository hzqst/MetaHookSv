#pragma once

#include <vector>

#define BLOB_ALGORITHM 0x12345678
#define BLOB_LOAD_BASE 0x1900000
#define BLOB_LOAD_END 0x2400000

typedef struct BlobInfo_s
{
	char m_szPath[10];
	char m_szDescribe[32];
	char m_szCompany[22];
	DWORD m_dwAlgorithm;
}BlobInfo_t;

typedef struct BlobHeader_s
{
	DWORD m_dwCheckSum;
	WORD m_wSectionCount;
	DWORD m_dwExportPoint;
	DWORD m_dwImageBase;
	DWORD m_dwEntryPoint;
	DWORD m_dwImportTable;
}BlobHeader_t;

typedef struct BlobSection_s
{
	DWORD m_dwVirtualAddress;
	DWORD m_dwVirtualSize;
	DWORD m_dwDataSize;
	DWORD m_dwDataAddress;
	BOOL m_bIsSpecial;
}BlobSection_t;

#define MAX_BLOB_IMPORT_LOADLIBRARY 64

typedef struct
{
	BlobHeader_t BlobHeader;

	ULONG_PTR SpecialAddress;
	ULONG ImageSize;

	PVOID TextBase;
	ULONG TextSize;

	PVOID DataBase;
	ULONG DataSize;

	int NumLoadLibraryRefs;
	HMODULE LoadLibraryRefs[MAX_BLOB_IMPORT_LOADLIBRARY];
}BlobModule_t;

void BlobRunFrame(void);
void InitBlobThreadManager(void);
void ShutdownBlobThreadManager(void);
void BlobWaitForAliveThreadsToShutdown(void);
void BlobWaitForClosedThreadsToShutdown(void);
BOOL FIsBlob(const char* szFileName);
BlobHandle_t LoadBlobFromBuffer(BYTE* pBuffer, DWORD dwBufferSize, PVOID BlobSectionBase, ULONG BlobSectionSize);
BlobHandle_t LoadBlobFile(const char* szFileName, PVOID BlobSectionBase, ULONG BlobSectionSize);
BlobHeader_t *GetBlobHeader(BlobHandle_t hBlob);
BOOL RunDllMainForBlob(BlobHandle_t hBlob, DWORD dwReason);
void RunExportEntryForBlob(BlobHandle_t hBlob, void** pv);
void FreeBlobModule(BlobHandle_t hBlob);
PVOID GetBlobModuleImageBase(BlobHandle_t hBlob);
ULONG GetBlobModuleImageSize(BlobHandle_t hBlob);
ULONG_PTR GetBlobModuleSpecialAddress(BlobHandle_t hBlob);
PVOID GetBlobLoaderSection(PVOID ImageBase, ULONG* BlobSectionSize);
PVOID GetBlobSectionByName(BlobHandle_t hBlob, const char* SectionName, ULONG* SectionSize);