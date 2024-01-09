#pragma once

#include <string>
#include <vector>

#define BLOB_ALGORITHM 0x12345678
#define BLOB_LOAD_BASE 0x1900000
#define BLOB_LOAD_END 0x2400000
#define BLOB_LOAD_CLIENT_BASE 0x1900000

BOOL FIsBlob(const char* szFileName);
BlobHandle_t LoadBlobFromBuffer(BYTE* pBuffer, DWORD dwBufferSize, PVOID BlobSectionBase, ULONG BlobSectionSize);
BlobHandle_t LoadBlobFile(const char* szFileName, PVOID BlobSectionBase, ULONG BlobSectionSize);
ULONG_PTR GetBlobHeaderExportPoint(BlobHandle_t hBlob);
BOOL RunDllMainForBlob(BlobHandle_t hBlob, DWORD dwReason);
void RunExportEntryForBlob(BlobHandle_t hBlob, void** pv);
void FreeBlobModule(BlobHandle_t hBlob);
PVOID GetBlobModuleImageBase(BlobHandle_t hBlob);
ULONG GetBlobModuleImageSize(BlobHandle_t hBlob);
ULONG_PTR GetBlobModuleSpecialAddress(BlobHandle_t hBlob);
PVOID GetBlobLoaderSection(PVOID ImageBase, ULONG* BlobSectionSize);
PVOID GetBlobSectionByName(BlobHandle_t hBlob, const char* SectionName, ULONG* SectionSize);
hook_t* MH_BlobIATHook(BlobHandle_t hBlob, const char* pszModuleName, const char* pszFuncName, void* pNewFuncAddr, void** pOrginalCall);
bool MH_BlobHasImport(BlobHandle_t hBlob, const char* pszModuleName);
bool MH_BlobHasImportEx(BlobHandle_t hBlob, const char* pszModuleName, const char* pszFuncName);
//BlobLoader
void BlobLoaderAddBlob(BlobHandle_t hBlob);
void BlobLoaderRemoveBlob(BlobHandle_t hBlob);
BlobHandle_t BlobLoaderFindBlobByImageBase(PVOID ImageBase);
BlobHandle_t BlobLoaderFindBlobByVirtualAddress(PVOID VirtualAddress);