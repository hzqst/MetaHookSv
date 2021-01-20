#ifndef IFILESYSTEM_H
#define IFILESYSTEM_H

#ifdef _WIN32
#pragma once
#endif

typedef void *FileHandle_t;
typedef int FileFindHandle_t;
typedef int WaitForResourcesHandle_t;

enum FileSystemSeek_t
{
	FILESYSTEM_SEEK_HEAD = 0,
	FILESYSTEM_SEEK_CURRENT,
	FILESYSTEM_SEEK_TAIL,
};

enum
{
	FILESYSTEM_INVALID_FIND_HANDLE = -1
};

enum FileWarningLevel_t
{
	FILESYSTEM_WARNING = -1,
	FILESYSTEM_WARNING_QUIET = 0,
	FILESYSTEM_WARNING_REPORTUNCLOSED,
	FILESYSTEM_WARNING_REPORTUSAGE,
	FILESYSTEM_WARNING_REPORTALLACCESSES,
	FILESYSTEM_WARNING_REPORTALLACCESSES_READ,
	FILESYSTEM_WARNING_REPORTALLACCESSES_READWRITE,
	FILESYSTEM_WARNING_REPORTALLACCESSES_ASYNC,
};

#define FILESYSTEM_INVALID_HANDLE (FileHandle_t)NULL

class IFileSystem : public IBaseInterface
{
public:
	virtual void Mount(void) = 0;
	virtual void Unmount(void) = 0;
	virtual void RemoveAllSearchPaths(void) = 0;
	virtual void AddSearchPath(const char *pPath, const char *pathID = 0) = 0;
	virtual bool RemoveSearchPath(const char *pPath) = 0;
	virtual void RemoveFile(const char *pRelativePath, const char *pathID = 0) = 0;
	virtual void CreateDirHierarchy(const char *path, const char *pathID = 0) = 0;
	virtual bool FileExists(const char *pFileName) = 0;
	virtual bool IsDirectory(const char *pFileName) = 0;
	virtual FileHandle_t Open(const char *pFileName, const char *pOptions, const char *pathID = 0) = 0;
	virtual void Close(FileHandle_t file) = 0;
	virtual void Seek(FileHandle_t file, int pos, FileSystemSeek_t seekType) = 0;
	virtual unsigned Tell(FileHandle_t file) = 0;
	virtual unsigned Size(FileHandle_t file) = 0;
	virtual unsigned Size(const char *pFileName) = 0;
	virtual long GetFileTime(const char *pFileName) = 0;
	virtual void FileTimeToString(char *pStrip, int maxCharsIncludingTerminator, long fileTime) = 0;
	virtual bool IsOk(FileHandle_t file) = 0;
	virtual void Flush(FileHandle_t file) = 0;
	virtual bool EndOfFile(FileHandle_t file) = 0;
	virtual int Read(void *pOutput, int size, FileHandle_t file) = 0;
	virtual int Write(void const *pInput, int size, FileHandle_t file) = 0;
	virtual char *ReadLine(char *pOutput, int maxChars, FileHandle_t file) = 0;
	virtual int FPrintf(FileHandle_t file, char *pFormat, ...) = 0;
	virtual char *GetReadBuffer(FileHandle_t file, char *pBuffer) = 0;
	virtual void ReleaseReadBuffer(FileHandle_t file, char *pBuffer) = 0;
	virtual const char *FindFirst(const char *pWildCard, FileFindHandle_t *pHandle, const char *pathID = 0) = 0;
	virtual const char *FindNext(FileFindHandle_t handle) = 0;
	virtual bool FindIsDirectory(FileFindHandle_t handle) = 0;
	virtual void FindClose(FileFindHandle_t handle) = 0;
	virtual void GetLocalCopy(const char *pFileName) = 0;
	virtual const char *GetLocalPath(const char *pFileName, char *pLocalPath, int maxlen) = 0;
	virtual char *ParseFile(char *data, char *token, bool *wasquoted) = 0;
	virtual bool FullPathToRelativePath(const char *pFullpath, char *pRelative) = 0;
	virtual bool GetCurrentDirectory(char *pDirectory, int maxlen) = 0;
	virtual void PrintOpenedFiles(void) = 0;
	virtual void SetWarningFunc(void (*pfnWarning)(const char *fmt, ...)) = 0;
	virtual void SetWarningLevel(FileWarningLevel_t level) = 0;
	virtual void LogLevelLoadStarted(const char *name) = 0;
	virtual void LogLevelLoadFinished(const char *name) = 0;
	virtual int HintResourceNeed(const char *hintlist, int forgetEverything) = 0;
	virtual int PauseResourcePreloading(void) = 0;
	virtual int ResumeResourcePreloading(void) = 0;
	virtual int SetVBuf(FileHandle_t stream, char *buffer, int mode, long size) = 0;
	virtual void GetInterfaceVersion(char *p, int maxlen) = 0;
	virtual bool IsFileImmediatelyAvailable(const char *path) = 0;
	virtual void *WaitForResources(const char *pFileName) = 0;
	virtual bool GetWaitForResourcesProgress(WaitForResourcesHandle_t handle, float *progress, bool *complete) = 0;
	virtual void CancelWaitForResources(WaitForResourcesHandle_t handle) = 0;
	virtual bool IsAppReadyForOfflinePlay(void) = 0;
	virtual void AddPackFile(const char *pPath, const char *pathID = 0) = 0;
	virtual void *OpenFromCacheForRead(const char *pFileName, const char *pOptions, const char *pathID = 0) = 0;
};

#define FILESYSTEM_INTERFACE_VERSION "VFileSystem009"

#endif