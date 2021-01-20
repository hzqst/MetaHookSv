#include "filesystem.h"
#include <windows.h>
#include <malloc.h>

extern IFileSystem *g_pFullFileSystem;

bool IFileSystem::FileExists(const char *pFileName, const char *pPathID)
{
	return (GetFileAttributesA(pFileName) != INVALID_FILE_ATTRIBUTES);
}

bool IFileSystem::IsFileWritable(char const *pFileName, const char *pPathID)
{
	return true;
}

bool IFileSystem::SetFileWritable(char const *pFileName, bool writable, const char *pPathID)
{
	return true;
}

int IFileSystem::ReadEx(void* pOutput, int sizeDest, int size, FileHandle_t file)
{
	return Read(pOutput, size, file);
}

bool IFileSystem::GetOptimalIOConstraints(FileHandle_t hFile, unsigned *pOffsetAlign, unsigned *pSizeAlign, unsigned *pBufferAlign)
{
	return false;
}

unsigned IFileSystem::GetOptimalReadSize(FileHandle_t hFile, unsigned nLogicalSize)
{
	return Size(hFile);
}

void *IFileSystem::AllocOptimalReadBuffer(FileHandle_t hFile, unsigned nSize, unsigned nOffset)
{
	return malloc(nSize);
}

void IFileSystem::FreeOptimalReadBuffer(void *buffer)
{
	return free(buffer);
}

const char *IFileSystem::RelativePathToFullPath(const char *pFileName, const char *pPathID, char *pLocalPath, int localPathBufferSize, PathTypeFilter_t pathFilter, PathTypeQuery_t *pPathType)
{
	pLocalPath[0] = 0;
	return NULL;
}

bool IFileSystem::IsDirectory(const char *pFileName, const char *pathID)
{
	DWORD dwAttributes = GetFileAttributesA(pFileName);
	return ((dwAttributes != -1) && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool IFileSystem::GetFileTypeForFullPath(char const *pFullPath, wchar_t *buf, size_t bufSizeInBytes)
{
	buf[0] = 0;
	return false;
}

bool IFileSystem::FullPathToRelativePathEx(const char *pFullpath, const char *pPathId, char *pRelative, int maxlen)
{
	pRelative[0] = 0;
	return false;
}

const char *IFileSystem::FindFirstEx(const char *pWildCard, const char *pPathID, FileFindHandle_t *pHandle)
{
	return FindFirst(pWildCard, pHandle, pPathID);;
}