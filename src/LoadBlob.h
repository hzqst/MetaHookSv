#pragma once

#include <vector>

#define BLOB_ALGORITHM 0x12345678
#define BLOB_ENGINE_BASE 0x1D00000
#define BLOB_ENGINE_SIZE_ESTIMATE 0x1230000

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

class CBlobModule
{
public:
	BlobHeader_t BlobHeader;

	ULONG ImageSize;

	PVOID TextBase;
	ULONG TextSize;

	PVOID DataBase;
	ULONG DataSize;

	std::vector<HMODULE> LoadLibraryRefs;
};

BOOL FIsBlob(const char* szFileName);
BlobHandle_t LoadBlobFile(const char* szFileName, PVOID BlobSectionBase, ULONG BlobSectionSize);
BlobHeader_t *GetBlobHeader(BlobHandle_t hBlob);
BOOL RunDllMainForBlob(BlobHandle_t hBlob, DWORD dwReason);
void RunExportEntryForBlob(BlobHandle_t hBlob, void** pv);
void FreeBlobModule(BlobHandle_t hBlob);
PVOID GetBlobModuleImageBase(BlobHandle_t hBlob);
ULONG GetBlobModuleImageSize(BlobHandle_t hBlob);
PVOID GetBlobLoaderSection(PVOID ImageBase, ULONG* BlobSectionSize);
PVOID GetBlobSectionByName(BlobHandle_t hBlob, const char* SectionName, ULONG* SectionSize);