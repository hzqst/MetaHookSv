#include <windows.h>
#include <interface.h>
#include "IFileSystem.h"
#include "metahook.h"
#include "LoadBlob.h"

PVOID MH_GetSectionByName(PVOID ImageBase, const char* SectionName, ULONG* SectionSize);
void* MH_SearchPattern(void* pStartSearch, DWORD dwSearchLen, const char* pPattern, DWORD dwPatternLen);

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

BlobHeader_t *GetBlobHeader(BlobHandle_t hBlob)
{
	auto pBlobModule = (CBlobModule*)hBlob;

	return &pBlobModule->BlobHeader;
}

BOOL FIsBlob(const char *szFileName)
{
	FileHandle_t hFile = FILESYSTEM_ANY_OPEN(szFileName, "rb");

	if (hFile == FILESYSTEM_INVALID_HANDLE)
		return FALSE;

	BlobInfo_t info = {0};
	FILESYSTEM_ANY_READ(&info, sizeof(BlobInfo_t), hFile);
	FILESYSTEM_ANY_CLOSE(hFile);

	if (info.m_dwAlgorithm != BLOB_ALGORITHM)
		return FALSE;

	return TRUE;
}

BOOL RunDllMainForBlob(BlobHandle_t hBlob, DWORD dwReason)
{
	auto pBlobModule = (CBlobModule*)hBlob;

	return ((BOOL(WINAPI*)(HINSTANCE, DWORD, void*))(pBlobModule->BlobHeader.m_dwEntryPoint))(0, dwReason, 0);
}

void RunExportEntryForBlob(BlobHandle_t hBlob, void** pv)
{
	auto pBlobModule = (CBlobModule*)hBlob;

	((void (*)(void**))(pBlobModule->BlobHeader.m_dwExportPoint))(pv);
}

bool BlobVerifyRange(PVOID Ptr, ULONG Size, PVOID ValidBase, ULONG ValidSize)
{
	if ((ULONG_PTR)Ptr >= (ULONG_PTR)ValidBase && (ULONG_PTR)Ptr + Size < (ULONG_PTR)ValidBase + ValidSize)
	{
		return true;
	}

	return false;
}

BlobHandle_t LoadBlobFromBuffer(BYTE* pBuffer, DWORD dwBufferSize, PVOID BlobSectionBase, ULONG BlobSectionSize)
{
	auto pBlobModule = new (std::nothrow) CBlobModule;

	if (!pBlobModule)
		return NULL;

	memset(&pBlobModule->BlobHeader, 0, sizeof(BlobHeader_t));
	pBlobModule->ImageSize = 0;
	pBlobModule->TextBase = 0;
	pBlobModule->TextSize = 0;
	pBlobModule->DataBase = 0;
	pBlobModule->DataSize = 0;

	BYTE bXor = 0x57;

	for (size_t i = sizeof(BlobInfo_t); i < dwBufferSize; i++)
	{
		pBuffer[i] ^= bXor;
		bXor += pBuffer[i] + 0x57;
	}

	const auto pHeader = (BlobHeader_t*)(pBuffer + sizeof(BlobInfo_t));

	if (!BlobVerifyRange(pHeader, sizeof(BlobHeader_t), pBuffer, dwBufferSize))
	{
		FreeBlobModule(pBlobModule);
		return NULL;
	}

	memcpy(&pBlobModule->BlobHeader, pHeader, sizeof(BlobHeader_t));

	pBlobModule->BlobHeader.m_dwExportPoint ^= 0x7A32BC85;
	pBlobModule->BlobHeader.m_dwImageBase ^= 0x49C042D1;
	pBlobModule->BlobHeader.m_dwEntryPoint -= 12;
	pBlobModule->BlobHeader.m_dwImportTable ^= 0x872C3D47;

	const auto pSection = (BlobSection_t*)(pBuffer + sizeof(BlobInfo_t) + sizeof(BlobHeader_t));

	if (!BlobVerifyRange(pSection, sizeof(BlobSection_t) * pBlobModule->BlobHeader.m_wSectionCount, pBuffer, dwBufferSize))
	{
		FreeBlobModule(pBlobModule);
		return NULL;
	}

	for (WORD j = 0; j <= pBlobModule->BlobHeader.m_wSectionCount; j++)
	{
		auto VirtualBase = (BYTE*)(pSection[j].m_dwVirtualAddress);
		auto VirtualSize = pSection[j].m_dwVirtualSize;

		if (!BlobVerifyRange(VirtualBase, VirtualSize, BlobSectionBase, BlobSectionSize))
		{
			FreeBlobModule(pBlobModule);
			return NULL;
		}

		if (VirtualSize > pSection[j].m_dwDataSize)
		{
			memset(VirtualBase + pSection[j].m_dwDataSize, 0, VirtualSize - pSection[j].m_dwDataSize);
		}

		memcpy(VirtualBase, pBuffer + pSection[j].m_dwDataAddress, pSection[j].m_dwDataSize);

		if (j == 0)
		{
			pBlobModule->TextBase = VirtualBase;
			pBlobModule->TextSize = VirtualSize;
		}
		
		if (j != 0 && VirtualSize > 0x10000)
		{
			if (MH_SearchPattern(VirtualBase, VirtualSize, "HeapAlloc", sizeof("HeapAlloc")) &&
				MH_SearchPattern(VirtualBase, VirtualSize, "HeapFree", sizeof("HeapFree")))
			{
				
			}
			else
			{
				pBlobModule->DataBase = VirtualBase;
				pBlobModule->DataSize = VirtualSize;
			}
		}

		if ((ULONG_PTR)pBlobModule->BlobHeader.m_dwImageBase + pBlobModule->ImageSize < (ULONG_PTR)VirtualBase + VirtualSize)
		{
			pBlobModule->ImageSize = (ULONG_PTR)VirtualBase + VirtualSize - (ULONG_PTR)pBlobModule->BlobHeader.m_dwImageBase;
		}
	}

	if (!BlobVerifyRange((PVOID)pBlobModule->BlobHeader.m_dwImageBase, pBlobModule->ImageSize, BlobSectionBase, BlobSectionSize))
	{
		FreeBlobModule(pBlobModule);
		return NULL;
	}

	PIMAGE_IMPORT_DESCRIPTOR pImport = (PIMAGE_IMPORT_DESCRIPTOR)pBlobModule->BlobHeader.m_dwImportTable;

	if (!BlobVerifyRange((PVOID)pImport, sizeof(IMAGE_IMPORT_DESCRIPTOR), BlobSectionBase, BlobSectionSize))
	{
		FreeBlobModule(pBlobModule);
		return NULL;
	}

	while (pImport->Name)
	{
		auto pszDllName = (const char*)(pBlobModule->BlobHeader.m_dwImageBase + pImport->Name);

		if (!BlobVerifyRange((PVOID)pszDllName, 1, BlobSectionBase, BlobSectionSize))
		{
			FreeBlobModule(pBlobModule);
			return NULL;
		}

		HMODULE hProcDll = LoadLibraryA(pszDllName);

		if (!hProcDll)
		{
			FreeBlobModule(pBlobModule);
			return NULL;
		}

		pBlobModule->LoadLibraryRefs.push_back(hProcDll);

		PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)(pBlobModule->BlobHeader.m_dwImageBase + pImport->FirstThunk);
		
		if (!BlobVerifyRange((PVOID)pThunk, sizeof(IMAGE_THUNK_DATA), BlobSectionBase, BlobSectionSize))
		{
			FreeBlobModule(pBlobModule);
			return NULL;
		}

		while (pThunk->u1.Function)
		{
			const char* pszProcName = NULL;

			if (IMAGE_SNAP_BY_ORDINAL(pThunk->u1.Ordinal))
			{
				pszProcName = (const char*)((LONG)pThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG32 - 1);
			}
			else
			{
				auto pName = (IMAGE_IMPORT_BY_NAME*)(pBlobModule->BlobHeader.m_dwImageBase + pThunk->u1.AddressOfData);

				if (!BlobVerifyRange((PVOID)pName, sizeof(IMAGE_IMPORT_BY_NAME), BlobSectionBase, BlobSectionSize))
				{
					FreeBlobModule(pBlobModule);
					return NULL;
				}

				pszProcName = (const char*)pName->Name;

				if (!BlobVerifyRange((PVOID)pszProcName, 1, BlobSectionBase, BlobSectionSize))
				{
					FreeBlobModule(pBlobModule);
					return NULL;
				}
			}

			if (!pszProcName)
			{
				FreeBlobModule(pBlobModule);
				return NULL;
			}

			pThunk->u1.AddressOfData = (DWORD)GetProcAddress(hProcDll, pszProcName);

			pThunk++;

			if (!BlobVerifyRange((PVOID)pThunk, sizeof(IMAGE_THUNK_DATA), BlobSectionBase, BlobSectionSize))
			{
				FreeBlobModule(pBlobModule);
				return NULL;
			}
		}

		pImport++;

		if (!BlobVerifyRange((PVOID)pImport, sizeof(IMAGE_IMPORT_DESCRIPTOR), BlobSectionBase, BlobSectionSize))
		{
			FreeBlobModule(pBlobModule);
			return NULL;
		}
	}

	return pBlobModule;
}

BlobHandle_t LoadBlobFile(const char *szFileName, PVOID BlobSectionBase, ULONG BlobSectionSize)
{
	BlobHandle_t hBlob = NULL;

	FileHandle_t hFile = FILESYSTEM_ANY_OPEN(szFileName, "rb");

	if (hFile != 0)
	{
		FILESYSTEM_ANY_SEEK(hFile, 0, FILESYSTEM_SEEK_TAIL);
		ULONG dwBufferSize = FILESYSTEM_ANY_TELL(hFile);
		FILESYSTEM_ANY_SEEK(hFile, 0, FILESYSTEM_SEEK_HEAD);

		auto pBuffer = (BYTE*)malloc(dwBufferSize);

		if (pBuffer)
		{
			FILESYSTEM_ANY_READ(pBuffer, dwBufferSize, hFile);

			hBlob = LoadBlobFromBuffer(pBuffer, dwBufferSize, BlobSectionBase, BlobSectionSize);

			free(pBuffer);
		}

		FILESYSTEM_ANY_CLOSE(hFile);
	}

	return hBlob;
}

void FreeBlobModule(BlobHandle_t hBlob)
{
	auto pBlobModule = (CBlobModule *)hBlob;

	for (auto hModule : pBlobModule->LoadLibraryRefs)
	{
		FreeLibrary(hModule);
	}

	memset((void*)pBlobModule->BlobHeader.m_dwImageBase, 0, pBlobModule->ImageSize);

	delete pBlobModule;
}

PVOID GetBlobModuleImageBase(BlobHandle_t hBlob)
{
	auto pBlobModule = (CBlobModule*)hBlob;

	return (PVOID)pBlobModule->BlobHeader.m_dwImageBase;
}

ULONG GetBlobModuleImageSize(BlobHandle_t hBlob)
{
	auto pBlobModule = (CBlobModule*)hBlob;

	return pBlobModule->ImageSize;
}

PVOID GetBlobSectionByName(BlobHandle_t hBlob, const char* SectionName, ULONG* SectionSize)
{
	auto pBlobModule = (CBlobModule*)hBlob;

	if (0 == memcmp(SectionName, ".text", sizeof(".text")))
	{
		if (SectionSize)
			*SectionSize = pBlobModule->TextSize;

		return pBlobModule->TextBase;
	}

	if (0 == memcmp(SectionName, ".data", sizeof(".data")))
	{
		if (SectionSize)
			*SectionSize = pBlobModule->DataSize;

		return pBlobModule->DataBase;
	}

	return NULL;
}

PVOID GetBlobLoaderSection(PVOID ImageBase, ULONG *BlobSectionSize)
{
	ULONG SectionSize = 0;
	auto SectionBase = MH_GetSectionByName(ImageBase, ".blob", &SectionSize);

	if (!SectionBase)
		return NULL;

	if(SectionBase > (PVOID)BLOB_ENGINE_BASE)
		return NULL;

	if ((ULONG_PTR)SectionBase + SectionSize < (ULONG_PTR)BLOB_ENGINE_SIZE_ESTIMATE)
		return NULL;

	*BlobSectionSize = SectionSize;
	return SectionBase;
}