#pragma once

#include <interface.h>
#include <stdint.h>

enum class UtilAssetsIntegrityCheckReason
{
    OK = 0,
    Unknown,
    InvalidFormat,
    SizeTooLarge,
    SizeTooSmall,
    BogusHeader,
    VersionMismatch,
    OutOfBound,
};

class UtilAssetsIntegrityCheckResult
{
public:
    UtilAssetsIntegrityCheckResult()
    {
        ReasonStr[0] = 0;
    }
public:
    char ReasonStr[256];
};

class UtilAssetsIntegrityCheckResult_StudioModel : public UtilAssetsIntegrityCheckResult
{
public:
    
};

class UtilAssetsIntegrityCheckResult_BMP : public UtilAssetsIntegrityCheckResult
{
public:
    UtilAssetsIntegrityCheckResult_BMP() : UtilAssetsIntegrityCheckResult()
    {
        MaxWidth = 0;
        MaxHeight = 0;
        MaxSize = 0;
    }
public:


    size_t MaxWidth;
    size_t MaxHeight;
    size_t MaxSize;
};

class IUtilAssetsIntegrity : public IBaseInterface
{
public:
    virtual UtilAssetsIntegrityCheckReason CheckStudioModel(const void *buf, size_t bufSize, UtilAssetsIntegrityCheckResult_StudioModel *checkResult) = 0;
    virtual UtilAssetsIntegrityCheckReason Check8bitBMP(const void* buf, size_t bufSize, UtilAssetsIntegrityCheckResult_BMP* checkResult) = 0;
};

IUtilAssetsIntegrity* UtilAssetsIntegrity();

#define UTIL_ASSETS_INTEGRITY_INTERFACE_VERSION "UtilAssetsIntegrityAPI_001"