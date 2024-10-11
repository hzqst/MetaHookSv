#include <windows.h>
#include <interface.h>
#include <mutex>
#include "IFileSystem.h"
#include "metahook.h"
#include "LoadBlob.h"

PVOID MH_GetSectionByName(PVOID ImageBase, const char* SectionName, ULONG* SectionSize);
void* MH_SearchPattern(void* pStartSearch, DWORD dwSearchLen, const char* pPattern, DWORD dwPatternLen);
void* MH_SearchPatternNoWildCard(void* pStartSearch, DWORD dwSearchLen, const char* pPattern, DWORD dwPatternLen);
void MH_SysError(const char* fmt, ...);

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

static std::mutex g_BlobLoaderLock;
static std::vector<BlobHandle_t> g_LoadedBlobs;

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

typedef struct BlobImportEntry_s
{
	BlobImportEntry_s(ULONG_PTR* a1, HMODULE a2, const char* a3, const char* a4) : ThunkFunction(a1), hProcDll(a2), DllName(a3), FunctionName(a4)
	{
	};
	ULONG_PTR* ThunkFunction;
	HMODULE hProcDll;
	std::string DllName;
	std::string FunctionName;
}BlobImportEntry_t;

typedef struct BlobModule_s
{
	BlobHeader_t BlobHeader;

	ULONG_PTR SpecialAddress;
	ULONG ImageSize;

	PVOID TextBase;
	ULONG TextSize;

	PVOID DataBase;
	ULONG DataSize;

	PVOID RDataBase;
	ULONG RDataSize;

	std::vector<HMODULE> LoadLibraryRefs;
	std::vector<BlobImportEntry_t> ImportEntries;
}BlobModule_t;

void BlobLoaderAddBlob(BlobHandle_t hBlob)
{
	std::lock_guard<std::mutex> lock(g_BlobLoaderLock);

	g_LoadedBlobs.emplace_back(hBlob);
}

void BlobLoaderRemoveBlob(BlobHandle_t hBlob)
{
	std::lock_guard<std::mutex> lock(g_BlobLoaderLock);

	for (auto itor = g_LoadedBlobs.begin(); itor != g_LoadedBlobs.end();)
	{
		if ((*itor) == hBlob)
		{
			itor = g_LoadedBlobs.erase(itor);
			return;
		}
		itor++;
	}
}

BlobHandle_t BlobLoaderFindBlobByImageBase(PVOID ImageBase)
{
	std::lock_guard<std::mutex> lock(g_BlobLoaderLock);

	for (auto hBlob : g_LoadedBlobs)
	{
		if (ImageBase == GetBlobModuleImageBase(hBlob))
		{
			return hBlob;
		}
	}

	return NULL;
}

BlobHandle_t BlobLoaderFindBlobByVirtualAddress(PVOID VirtualAddress)
{
	std::lock_guard<std::mutex> lock(g_BlobLoaderLock);

	for (auto hBlob : g_LoadedBlobs)
	{
		auto ImageBase = GetBlobModuleImageBase(hBlob);
		auto ImageSize = GetBlobModuleImageSize(hBlob);

		if (VirtualAddress >= ImageBase && VirtualAddress < (PUCHAR)ImageBase + ImageSize)
		{
			return hBlob;
		}
	}

	return NULL;
}

BlobHeader_t *GetBlobHeader(BlobHandle_t hBlob)
{
	auto pBlobModule = (BlobModule_t*)hBlob;

	return &pBlobModule->BlobHeader;
}

ULONG_PTR GetBlobHeaderExportPoint(BlobHandle_t hBlob)
{
	return (ULONG_PTR)GetBlobHeader(hBlob)->m_dwExportPoint;
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
	auto pBlobModule = (BlobModule_t*)hBlob;

	return ((BOOL(WINAPI*)(HINSTANCE, DWORD, void*))(pBlobModule->BlobHeader.m_dwEntryPoint))(0, dwReason, 0);
}

void RunExportEntryForBlob(BlobHandle_t hBlob, void** pv)
{
	auto pBlobModule = (BlobModule_t*)hBlob;

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

bool BlobVerifyStringRange(PVOID Ptr, ULONG Size, PVOID ValidBase, ULONG ValidSize)
{
	PCHAR pString = (PCHAR)Ptr;

	for (ULONG i = 0; i < Size; ++i)
	{
		if (!BlobVerifyRange(&pString[i], 1, ValidBase, ValidSize))
			return false;

		if (pString[i] == 0)
			return true;
	}

	return true;
}

BlobHandle_t LoadBlobFromBuffer(BYTE* pBuffer, DWORD dwBufferSize, PVOID BlobSectionBase, ULONG BlobSectionSize)
{
#if defined(METAHOOK_BLOB_SUPPORT) || defined(_DEBUG)
	auto pBlobModule = new (std::nothrow) BlobModule_t;

	if (!pBlobModule)
		return NULL;

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
	pBlobModule->SpecialAddress = 0;
	pBlobModule->ImageSize = 0;
	pBlobModule->TextBase = 0;
	pBlobModule->TextSize = 0;
	pBlobModule->DataBase = 0;
	pBlobModule->DataSize = 0;

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
				if (MH_SearchPattern(VirtualBase, VirtualSize, "Microsoft Visual C++ Runtime Library", sizeof("Microsoft Visual C++ Runtime Library") - 1) &&
					MH_SearchPattern(VirtualBase, VirtualSize, "JanFebMarAprMayJunJulAugSepOctNovDec", sizeof("JanFebMarAprMayJunJulAugSepOctNovDec") - 1))
				{
					pBlobModule->RDataBase = VirtualBase;
					pBlobModule->RDataSize = VirtualSize;
				}
			}
			else if(VirtualSize > 0x10000)
			{
				pBlobModule->DataBase = VirtualBase;
				pBlobModule->DataSize = VirtualSize;
			}
		}

		if ((ULONG_PTR)pBlobModule->BlobHeader.m_dwImageBase + pBlobModule->ImageSize < (ULONG_PTR)VirtualBase + VirtualSize)
		{
			pBlobModule->ImageSize = (ULONG_PTR)VirtualBase + VirtualSize - (ULONG_PTR)pBlobModule->BlobHeader.m_dwImageBase;
		}

		if (pSection[j].m_bIsSpecial)
		{
			pBlobModule->SpecialAddress = pSection[j].m_dwDataAddress;
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

		if (!BlobVerifyStringRange((PVOID)pszDllName, 64, BlobSectionBase, BlobSectionSize))
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

		pBlobModule->LoadLibraryRefs.emplace_back(hProcDll);

		PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)(pBlobModule->BlobHeader.m_dwImageBase + pImport->FirstThunk);
		
		if (!BlobVerifyRange((PVOID)pThunk, sizeof(IMAGE_THUNK_DATA), BlobSectionBase, BlobSectionSize))
		{
			FreeBlobModule(pBlobModule);
			return NULL;
		}

		while (pThunk->u1.Function)
		{
			bool bIsLoadByOrdinal = false;
			const char* pszProcName = NULL;

			if (IMAGE_SNAP_BY_ORDINAL(pThunk->u1.Ordinal))
			{
				pszProcName = (const char*)((LONG)pThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG32 - 1);
				bIsLoadByOrdinal = true;
			}
			else
			{
				auto pNameThunk = (IMAGE_IMPORT_BY_NAME*)(pBlobModule->BlobHeader.m_dwImageBase + pThunk->u1.AddressOfData);

				if (!BlobVerifyRange((PVOID)pNameThunk, sizeof(IMAGE_IMPORT_BY_NAME), BlobSectionBase, BlobSectionSize))
				{
					FreeBlobModule(pBlobModule);
					return NULL;
				}

				pszProcName = (const char*)pNameThunk->Name;

				if (!BlobVerifyStringRange((PVOID)pszProcName, 64, BlobSectionBase, BlobSectionSize))
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

			if (!bIsLoadByOrdinal)
			{
				pBlobModule->ImportEntries.emplace_back(&pThunk->u1.AddressOfData, hProcDll, pszDllName, pszProcName);
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
#else
	return NULL;
#endif
}

BlobHandle_t LoadBlobFile(const char *szFileName, PVOID BlobSectionBase, ULONG BlobSectionSize)
{
#if defined(METAHOOK_BLOB_SUPPORT) || defined(_DEBUG)

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
#else
	return NULL;
#endif
}

void FreeBlobModule(BlobHandle_t hBlob)
{
	auto pBlobModule = (BlobModule_t *)hBlob;

	//Shutdown CRT for it
	RunDllMainForBlob(hBlob, DLL_PROCESS_DETACH);

	for (int i = (int)pBlobModule->LoadLibraryRefs.size() - 1; i >= 0; --i)
	{
		FreeLibrary(pBlobModule->LoadLibraryRefs[i]);
	}

	//Just to check if any thread is not shutting down
#ifdef _DEBUG
	memset((void*)pBlobModule->BlobHeader.m_dwImageBase, 0, pBlobModule->ImageSize);
#endif

	delete pBlobModule;
}

PVOID GetBlobModuleImageBase(BlobHandle_t hBlob)
{
	auto pBlobModule = (BlobModule_t*)hBlob;

	return (PVOID)pBlobModule->BlobHeader.m_dwImageBase;
}

ULONG GetBlobModuleImageSize(BlobHandle_t hBlob)
{
	auto pBlobModule = (BlobModule_t*)hBlob;

	return pBlobModule->ImageSize;
}

ULONG_PTR GetBlobModuleSpecialAddress(BlobHandle_t hBlob)
{
	auto pBlobModule = (BlobModule_t*)hBlob;

	return pBlobModule->SpecialAddress;
}

PVOID GetBlobSectionByName(BlobHandle_t hBlob, const char* SectionName, ULONG* SectionSize)
{
	auto pBlobModule = (BlobModule_t*)hBlob;

	if (0 == memcmp(SectionName, ".text\x0\x0\x0", sizeof(".text\x0\x0\x0") - 1))
	{
		if (SectionSize)
			*SectionSize = pBlobModule->TextSize;

		return pBlobModule->TextBase;
	}

	if (0 == memcmp(SectionName, ".data\x0\x0\x0", sizeof(".data\x0\x0\x0") - 1))
	{
		if (SectionSize)
			*SectionSize = pBlobModule->DataSize;

		return pBlobModule->DataBase;
	}

	if (0 == memcmp(SectionName, ".rdata\x0\x0", sizeof(".rdata\x0\x0") - 1))
	{
		if (SectionSize)
			*SectionSize = pBlobModule->RDataSize;

		return pBlobModule->RDataBase;
	}

	return NULL;
}

PVOID GetBlobLoaderSection(PVOID ImageBase, ULONG *BlobSectionSize)
{
	ULONG SectionSize = 0;
	auto SectionBase = MH_GetSectionByName(ImageBase, ".blob\0\0\0", &SectionSize);

	if (!SectionBase)
		return NULL;

	if(SectionBase > (PVOID)BLOB_LOAD_BASE)
		return NULL;

	if ((ULONG_PTR)SectionBase + SectionSize < (ULONG_PTR)BLOB_LOAD_END)
		return NULL;

	*BlobSectionSize = SectionSize;
	return SectionBase;
}

hook_t* MH_CreateIATHook(HMODULE hModule, BlobHandle_t hBlob, const char* pszModuleName, const char* pszFuncName, void* pNewFuncAddr, void** pOrginalCall, ULONG_PTR* pThunkFunction);

hook_t* MH_BlobIATHook(BlobHandle_t hBlob, const char* pszModuleName, const char* pszFuncName, void* pNewFuncAddr, void** pOrginalCall)
{
	auto pBlobModule = (BlobModule_t*)hBlob;

	for (const auto& entry : pBlobModule->ImportEntries)
	{
		if (!stricmp(entry.DllName.c_str(), pszModuleName) && !stricmp(entry.FunctionName.c_str(), pszFuncName))
		{
			return MH_CreateIATHook(NULL, hBlob, pszModuleName, pszFuncName, pNewFuncAddr, pOrginalCall, entry.ThunkFunction);
		}
	}

	return NULL;
}

bool MH_BlobHasImport(BlobHandle_t hBlob, const char* pszModuleName)
{
	auto pBlobModule = (BlobModule_t*)hBlob;

	for (const auto& entry : pBlobModule->ImportEntries)
	{
		if (!stricmp(entry.DllName.c_str(), pszModuleName))
		{
			return true;
		}
	}

	return false;
}

bool MH_BlobHasImportEx(BlobHandle_t hBlob, const char* pszModuleName, const char* pszFuncName)
{
	auto pBlobModule = (BlobModule_t*)hBlob;

	for (const auto& entry : pBlobModule->ImportEntries)
	{
		if (!stricmp(entry.DllName.c_str(), pszModuleName) && !stricmp(entry.FunctionName.c_str(), pszFuncName))
		{
			return true;
		}
	}

	return false;
}