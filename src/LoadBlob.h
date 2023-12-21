#define BLOB_ALGORITHM 0x12345678

typedef struct BlobInfo_s
{
	char m_szPath[10];
	char m_szDescribe[32];
	char m_szCompany[22];
	DWORD m_dwAlgorithm;
}
BlobInfo_t;

typedef struct BlobHeader_s
{
	DWORD m_dwCheckSum;
	WORD m_wSectionCount;
	DWORD m_dwExportPoint;
	DWORD m_dwImageBase;
	DWORD m_dwEntryPoint;
	DWORD m_dwImportTable;
}
BlobHeader_t;

typedef struct BlobSection_s
{
	DWORD m_dwVirtualAddress;
	DWORD m_dwVirtualSize;
	DWORD m_dwDataSize;
	DWORD m_dwDataAddress;
	BOOL m_bIsSpecial;
}
BlobSection_t;

typedef struct BlobFootprint_s
{
	HMODULE m_hDll;
}
BlobFootprint_t;

BlobHeader_t *GetBlobHeader(void);
BOOL FIsBlob(const char *pstFileName);
DWORD NLoadBlobFile(const char *pstFileName, BlobFootprint_t *pblobfootprint, void **pv);
DWORD LoadBlobFile(BYTE *pBuffer, BlobFootprint_t *pblobfootprint, void **pv, DWORD dwSize);
void FreeBlob(BlobFootprint_t *pblobfootprint);