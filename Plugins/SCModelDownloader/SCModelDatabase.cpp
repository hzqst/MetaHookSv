#include <metahook.h>
#include "plugins.h"

#include "UtilHTTPClient.h"
#include "UtilAssetsIntegrity.h"
#include "UtilAutoPtr.h"
#include "SCModelDatabase.h"

#include <string>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <format>

#include <rapidjson/document.h>
#include <ScopeExit/ScopeExit.h>

bool SCModel_ShouldDownloadLatest();
int SCModel_CDN();
void SCModel_ReloadModel(const char* name);

static unsigned int g_uiAllocatedTaskId = 0;

class ISCModelQueryInternal : public ISCModelQuery
{
public:
	virtual void AddRef() = 0;
	virtual void Release() = 0;

	virtual void OnResponding(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) = 0;
	virtual void OnFinish(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) = 0;
	virtual bool OnStreamComplete(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) = 0;
	virtual bool OnProcessPayload(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) = 0;
	virtual void OnReceiveChunk(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) = 0;
	virtual void OnFailure() = 0;

	virtual void RunFrame(float flCurrentAbsTime) = 0;
	virtual void StartQuery() = 0;
};

class ISCModelDatabaseInternal : public ISCModelDatabase
{
public:
	virtual bool IsModelResourceFileAvailable(const std::string& localFileNameBase, const std::string& localFileName) = 0;

	virtual bool IsAllRequiredFilesForModelAvailable(const std::string& localFileNameBase, bool bHasTModel) = 0;

	virtual void BuildQueryModelResource(
		int repoId,
		const std::string& localFileNameBase,
		const std::string& localFileName,
		const std::string& networkFileNameBase,
		const std::string& networkFileName,
		bool bHasTModel
	) = 0;

	virtual void OnModelFileWriteFinished(const std::string& localFileNameBase, bool bHasTModel) = 0;

	virtual bool OnDatabaseJSONAcquired(const char* data, size_t size, const char* sourceFile) = 0;

	virtual bool OnVersionsJSONAcquired(const char* data, size_t size, const char* sourceFile) = 0;

	virtual void DispatchQueryStateChangeCallback(ISCModelQuery* pQuery, SCModelQueryState newState) = 0;
};

ISCModelDatabaseInternal* SCModelDatabaseInternal();

typedef struct scmodel_s
{
	int size;
	int flags;
	int polys;
}scmodel_t;

static int SCModel_Hash(const std::string& name)
{
	int hash = 0;

	for (size_t i = 0; i < name.length(); i++) {
		char ch = (char)name[i];
		hash = ((hash << 5) - hash) + ch;
		hash = hash % 15485863; // prevent hash ever increasing beyond 31 bits
	}
	return hash;
}

static int UTIL_GetContentLength(IUtilHTTPResponse* ResponseInstance)
{
	char szContentLength[32]{};
	if (ResponseInstance->GetHeader("Content-Length", szContentLength, sizeof(szContentLength) - 1) && szContentLength[0])
	{
		return atoi(szContentLength);
	}

	return -1;
}

class CUtilHTTPCallbacks : public IUtilHTTPCallbacks
{
private:
	//No AutoPtr, because each ISCModelQueryInternal has their own IUtilHTTPCallbacks
	ISCModelQueryInternal* m_pQueryTask{};

public:
	CUtilHTTPCallbacks(ISCModelQueryInternal* p) : m_pQueryTask(p)
	{

	}

	void Destroy() override
	{
		delete this;
	}

	void OnResponseComplete(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		if (!RequestInstance->IsRequestSuccessful())
		{
			m_pQueryTask->OnFailure();
			return;
		}

		if (ResponseInstance->IsResponseError())
		{
			m_pQueryTask->OnFailure();
			return;
		}

		if (!RequestInstance->IsStream())
		{
			auto pPayload = ResponseInstance->GetPayload();

			if (!m_pQueryTask->OnProcessPayload(RequestInstance, ResponseInstance, (const void*)pPayload->GetBytes(), pPayload->GetLength()))
			{
				m_pQueryTask->OnFailure();
				return;
			}
		}
		else
		{
			if (!m_pQueryTask->OnStreamComplete(RequestInstance, ResponseInstance))
			{
				m_pQueryTask->OnFailure();
				return;
			}
		}
	}

	void OnUpdateState(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, UtilHTTPRequestState NewState) override
	{
		if (NewState == UtilHTTPRequestState::Responding)
		{
			m_pQueryTask->OnResponding(RequestInstance, ResponseInstance);
		}
		if (NewState == UtilHTTPRequestState::Finished)
		{
			m_pQueryTask->OnFinish(RequestInstance, ResponseInstance);
		}
	}

	void OnReceiveData(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* pData, size_t cbSize) override
	{
		//Only stream request has OnReceiveData
		m_pQueryTask->OnReceiveChunk(RequestInstance, ResponseInstance, pData, cbSize);
	}
};

class CSCModelQueryBase : public ISCModelQueryInternal
{
private:
	volatile long m_RefCount{};
	bool m_bResponding{};
	bool m_bFinished{};
	bool m_bFailed{};
	float m_flNextRetryTime{};
	unsigned int m_uiTaskId{};

protected:
	std::string m_Url;
	UtilHTTPRequestId_t m_RequestId{ UTILHTTP_REQUEST_INVALID_ID };

public:
	CSCModelQueryBase()
	{
		m_RefCount = 1;
		m_uiTaskId = g_uiAllocatedTaskId;
		g_uiAllocatedTaskId++;
	}

	~CSCModelQueryBase()
	{
#ifdef _DEBUG
		gEngfuncs.Con_DPrintf("SCModelQuery: deleting query \"%s\"\n", m_Url.c_str());
#endif
		if (m_RequestId != UTILHTTP_REQUEST_INVALID_ID)
		{
			UtilHTTPClient()->DestroyRequestById(m_RequestId);
			m_RequestId = UTILHTTP_REQUEST_INVALID_ID;
		}
	}

	void AddRef() override
	{
		InterlockedIncrement(&m_RefCount);
	}

	void Release() override
	{
		if (InterlockedDecrement(&m_RefCount) == 0)
		{
			delete this;
		}
	}

	const char* GetUrl() const override
	{
		return m_Url.c_str();
	}

	float GetProgress() const override
	{
		return -1;
	}

	bool IsFinished() const override
	{
		return m_bFinished;
	}

	bool IsFailed() const override
	{
		return m_bFailed;
	}

	bool NeedRetry() const override
	{
		return m_flNextRetryTime > 0;
	}

	SCModelQueryState GetState() const override
	{
		if (m_bFailed)
			return SCModelQueryState_Failed;

		if (m_bFinished)
			return SCModelQueryState_Finished;

		if (m_bResponding)
			return SCModelQueryState_Receiving;

		return SCModelQueryState_Querying;
	}

	unsigned int GetTaskId() const override
	{
		return m_uiTaskId;
	}

	void OnResponding(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		m_bResponding = true;

		SCModelDatabaseInternal()->DispatchQueryStateChangeCallback(this, GetState());
	}

	void OnFailure() override
	{
		m_bResponding = false;
		m_bFailed = true;
		m_flNextRetryTime = (float)gEngfuncs.GetAbsoluteTime() + 5.0f;

		SCModelDatabaseInternal()->DispatchQueryStateChangeCallback(this, GetState());
	}

	void OnFinish(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		m_bResponding = false;
		m_bFinished = true;

		SCModelDatabaseInternal()->DispatchQueryStateChangeCallback(this, GetState());
	}

	bool OnStreamComplete(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		return true;
	}

	bool OnProcessPayload(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		auto nContentLength = UTIL_GetContentLength(ResponseInstance);

		if (nContentLength >= 0 && size != nContentLength)
		{
			gEngfuncs.Con_Printf("[SCModelDownloader] Content-Length mismatch for \"%s\": expect %d , got %d !\n", m_Url.c_str(), nContentLength, size);
			return false;
		}

		//do nothing
		return true;
	}

	void OnReceiveChunk(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		//do nothing
	}

	void RunFrame(float flCurrentAbsTime) override
	{
		if (IsFailed() && flCurrentAbsTime > m_flNextRetryTime)
		{
			StartQuery();
		}
	}

	virtual void StartQuery()
	{
		if (m_RequestId != UTILHTTP_REQUEST_INVALID_ID)
		{
			UtilHTTPClient()->DestroyRequestById(m_RequestId);
			m_RequestId = UTILHTTP_REQUEST_INVALID_ID;
		}

		m_flNextRetryTime = 0;
		m_bFailed = false;
		m_bFinished = false;
	}
};

class CSCModelQueryModelResourceTask : public CSCModelQueryBase
{
public:
	int m_repoId{};
	std::string m_localFileNameBase;
	std::string m_localFileName;
	std::string m_networkFileNameBase;
	std::string m_networkFileName;
	bool m_bHasTModel{};
	std::string m_identifier;
	FileHandle_t m_hFileHandle{};
	size_t m_expectedFileSize{};
	size_t m_receivedFileSize{};

public:
	CSCModelQueryModelResourceTask(
		int repoId,
		const std::string& localFileNameBase,
		const std::string& localFileName,
		const std::string& networkFileNameBase,
		const std::string& networkFileName,
		bool bHasTModel) :
		CSCModelQueryBase(),

		m_repoId(repoId),
		m_localFileNameBase(localFileNameBase),
		m_localFileName(localFileName),
		m_networkFileNameBase(networkFileNameBase),
		m_networkFileName(networkFileName),
		m_bHasTModel(bHasTModel)
	{
		std::transform(m_networkFileNameBase.begin(), m_networkFileNameBase.end(), m_networkFileNameBase.begin(), ::tolower);
		std::transform(m_networkFileName.begin(), m_networkFileName.end(), m_networkFileName.begin(), ::tolower);

		m_identifier = std::format("/models/player/{0}", m_networkFileName);
	}

	~CSCModelQueryModelResourceTask()
	{
		if (m_hFileHandle)
		{
			FILESYSTEM_ANY_CLOSE(m_hFileHandle);
			m_hFileHandle = nullptr;
		}
	}

	float GetProgress() const override
	{
		if(m_expectedFileSize > 0)
			return (float)m_receivedFileSize / (float)m_expectedFileSize;

		return -1;
	}

	void StartQuery() override
	{
		CSCModelQueryBase::StartQuery();

		m_Url = std::format("https://wootdata.github.io/scmodels_data_{0}/models/player/{1}/{2}", m_repoId, m_networkFileNameBase, m_networkFileName);

		if (SCModel_CDN() == 1)
		{
			m_Url = std::format("https://cdn.jsdelivr.net/gh/wootdata/scmodels_data_{0}@master/models/player/{1}/{2}", m_repoId, m_networkFileNameBase, m_networkFileName);
		}

		auto pRequestInstance = UtilHTTPClient()->CreateAsyncStreamRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));

		if (!pRequestInstance)
		{
			CSCModelQueryBase::OnFailure();
			return;
		}

		m_receivedFileSize = 0;

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();

		SCModelDatabaseInternal()->DispatchQueryStateChangeCallback(this, GetState());
	}

	void OnResponding(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		CSCModelQueryBase::OnResponding(RequestInstance, ResponseInstance);

		m_receivedFileSize = 0;

		FILESYSTEM_ANY_CREATEDIR("models", "GAMEDOWNLOAD");
		FILESYSTEM_ANY_CREATEDIR("models/player", "GAMEDOWNLOAD");

		std::string filePathDir = std::format("models/player/{0}", m_localFileNameBase);
		FILESYSTEM_ANY_CREATEDIR(filePathDir.c_str(), "GAMEDOWNLOAD");

		std::string filePathTmp = std::format("models/player/{0}/{1}.tmp", m_localFileNameBase, m_localFileName);
		m_hFileHandle = FILESYSTEM_ANY_OPEN(filePathTmp.c_str(), "wb", "GAMEDOWNLOAD");

		if (m_hFileHandle)
		{
			gEngfuncs.Con_Printf("[SCModelDownloader] Start writing \"%s\" ...\n", filePathTmp.c_str());
		}
		else
		{
			gEngfuncs.Con_Printf("[SCModelDownloader] Failed to open temp file \"%s\" for writing!\n", filePathTmp.c_str());
			return;
		}
	}

	void OnReceiveChunk(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		if (!m_expectedFileSize)
		{
			auto nContentLength = UTIL_GetContentLength(ResponseInstance);

			if (nContentLength > 0)
				m_expectedFileSize = nContentLength;
		}

		if (m_hFileHandle)
		{
			FILESYSTEM_ANY_WRITE(data, size, m_hFileHandle);

			m_receivedFileSize += size;
		}

		SCModelDatabaseInternal()->DispatchQueryStateChangeCallback(this, SCModelQueryState_Receiving);
	}

	bool OnStreamComplete(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		bool bSuccess = false;

		std::string fileToDetele;

		if (m_hFileHandle)
		{
			FILESYSTEM_ANY_CLOSE(m_hFileHandle);

			m_hFileHandle = nullptr;

			std::string filePathTmp = std::format("models/player/{0}/{1}.tmp", m_localFileNameBase, m_localFileName);

			fileToDetele = filePathTmp;

			gEngfuncs.Con_Printf("[SCModelDownloader] Temp file \"%s\" downloaded.\n", filePathTmp.c_str());

			auto hFileHandleRead = FILESYSTEM_ANY_OPEN(filePathTmp.c_str(), "rb", "GAMEDOWNLOAD");

			if (hFileHandleRead)
			{
				SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(hFileHandleRead); };

				auto fileSize = FILESYSTEM_ANY_SIZE(hFileHandleRead);

				if (fileSize > 0)
				{
					if (m_expectedFileSize >= 0 && fileSize < m_expectedFileSize)
					{
						gEngfuncs.Con_Printf("[SCModelDownloader] Temp file \"%s\" size mismatch ! expect %d, got %d. The downloading progress might be interrupted.\n", filePathTmp.c_str(), m_expectedFileSize, fileSize);
						return false;
					}

					auto fileBuf = malloc(fileSize);

					if (fileBuf)
					{
						SCOPE_EXIT{ free(fileBuf); };

						if (FILESYSTEM_ANY_READ(fileBuf, fileSize, hFileHandleRead) == fileSize)
						{
							if (m_localFileName.ends_with(".mdl"))
							{
								UtilAssetsIntegrityCheckResult_StudioModel checkResult{};
								if (UtilAssetsIntegrityCheckReason::OK != UtilAssetsIntegrity()->CheckStudioModel(fileBuf, fileSize, &checkResult))
								{
									gEngfuncs.Con_Printf("[SCModelDownloader] File \"%s\" failed the integrity check!\n", filePathTmp.c_str());
									gEngfuncs.Con_Printf("[SCModelDownloader] Integrity check result: %s.\n", checkResult.ReasonStr);
									return false;
								}
							}
							else if (m_localFileName.ends_with(".bmp"))
							{
								/*
								void EngineSurface::drawSetTextureFile(int id,const char* filename,int hardwareFilter,bool forceReload)
								//....
										unsigned char buf[256*256*4];//Cannot excceed 256x256

								*/

								UtilAssetsIntegrityCheckResult_BMP checkResult{};
								checkResult.MaxWidth = 256;
								checkResult.MaxHeight = 256;
								checkResult.MaxSize = 256 * 256;

								if (UtilAssetsIntegrityCheckReason::OK != UtilAssetsIntegrity()->Check8bitBMP(fileBuf, fileSize, &checkResult))
								{
									gEngfuncs.Con_Printf("[SCModelDownloader] File \"%s\" failed the integrity check!\n", filePathTmp.c_str());
									gEngfuncs.Con_Printf("[SCModelDownloader] Integrity check result: %s.\n", checkResult.ReasonStr);
									return false;
								}
							}
							std::string filePath = std::format("models/player/{0}/{1}", m_localFileNameBase, m_localFileName);

							auto hFileHandleWrite = FILESYSTEM_ANY_OPEN(filePath.c_str(), "wb", "GAMEDOWNLOAD");

							if (hFileHandleWrite)
							{
								SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(hFileHandleWrite); };

								FILESYSTEM_ANY_WRITE(fileBuf, fileSize, hFileHandleWrite);

								bSuccess = true;
							}
							else
							{
								gEngfuncs.Con_Printf("[SCModelDownloader] Failed to open file \"%s\" for writing!\n", filePath.c_str());
							}
						}
						else
						{
							gEngfuncs.Con_Printf("[SCModelDownloader] Failed to read content from \"%s\" !\n", filePathTmp.c_str());
						}
					}
					else
					{
						gEngfuncs.Con_Printf("[SCModelDownloader] Failed to allocate % bytes from \"%s\" !\n", fileSize, filePathTmp.c_str());
					}
				}
				else
				{
					gEngfuncs.Con_Printf("[SCModelDownloader] File \"%s\" is empty !\n", filePathTmp.c_str());
				}
			}
			else
			{
				gEngfuncs.Con_Printf("[SCModelDownloader] Failed to open file \"%s\" for reading!\n", filePathTmp.c_str());
			}
		}

		if (!fileToDetele.empty())
		{
			FILESYSTEM_ANY_REMOVEFILE(fileToDetele.c_str(), "GAMEDOWNLOAD");
		}

		if (m_localFileName.ends_with(".mdl"))
		{
			SCModelDatabaseInternal()->OnModelFileWriteFinished(m_localFileNameBase, m_bHasTModel);
		}

		return bSuccess;
	}

	const char* GetName() const override
	{
		return "QueryModelResource";
	}

	const char* GetIdentifier() const override
	{
		return m_identifier.c_str();
	}
};

class CSCModelQueryTaskList : public CSCModelQueryBase
{
public:
	int m_repoId{};
	std::string m_lowerName;
	std::string m_localFileNameBase;
	std::string m_networkFileNameBase;
	bool m_bHasTModel{};

public:
	CSCModelQueryTaskList(const std::string& lowerName, const std::string& localFileNameBase) :
		CSCModelQueryBase(),
		m_repoId(SCModel_Hash(lowerName) % 32),
		m_lowerName(lowerName),
		m_localFileNameBase(localFileNameBase)
	{

	}

	void StartQuery() override
	{
		CSCModelQueryBase::StartQuery();

		m_Url = std::format("https://wootdata.github.io/scmodels_data_{0}/models/player/{1}/{1}.json", m_repoId, m_lowerName);

		if (SCModel_CDN() == 1)
		{
			//https://cdn.jsdelivr.net/gh/wootdata/scmodels_data_12@master/models/player/gfl_m14-c2_v2/gfl_m14-c2_v2.json
			m_Url = std::format("https://cdn.jsdelivr.net/gh/wootdata/scmodels_data_{0}@master/models/player/{1}/{1}.json", m_repoId, m_lowerName); 
		}

		auto pRequestInstance = UtilHTTPClient()->CreateAsyncRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));

		if (!pRequestInstance)
		{
			CSCModelQueryBase::OnFailure();
			return;
		}

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();

		SCModelDatabaseInternal()->DispatchQueryStateChangeCallback(this, GetState());
	}

	bool OnProcessPayload(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		if (!CSCModelQueryBase::OnProcessPayload(RequestInstance, ResponseInstance, data, size))
			return false;

		rapidjson::Document doc;

		doc.Parse((const char*)data, size);

		if (doc.HasParseError())
		{
			gEngfuncs.Con_Printf("[SCModelDownloader] Failed to parse model json response!\n");
			return false;
		}

		gEngfuncs.Con_Printf("[SCModelDownloader] Json for model \"%s\" acquired!\n", m_localFileNameBase.c_str());

		auto obj = doc.GetObj();

		bool bHasTModel = false;

		std::string networkFileNameBase = m_lowerName;

		auto json_t_model = doc.FindMember("t_model");
		if (json_t_model != doc.MemberEnd() && json_t_model->value.IsBool())
		{
			bHasTModel = json_t_model->value.GetBool();
		}

		auto json_name = doc.FindMember("name");
		if (json_name != doc.MemberEnd() && json_name->value.IsString())
		{
			std::string name = json_name->value.GetString();

			if (name.size() > 4 &&
				name[name.size() - 4] == '.' &&
				name[name.size() - 3] == 'm' &&
				name[name.size() - 2] == 'd' &&
				name[name.size() - 1] == 'l')
			{
				name = name.erase(name.size() - 4, 4);
			}

			if (0 == stricmp(networkFileNameBase.c_str(), name.c_str()))
			{
				networkFileNameBase = name;
			}
		}
#if 0
		size_t expectFileSize = 0;
		auto size_name = doc.FindMember("size"); 
		if (size_name != doc.MemberEnd() && size_name->value.IsInt())
		{
			expectFileSize = size_name->value.GetInt();
		}
#endif
		m_networkFileNameBase = networkFileNameBase;
		m_bHasTModel = bHasTModel;

		if (!m_networkFileNameBase.empty() &&
			!SCModelDatabaseInternal()->IsModelResourceFileAvailable(m_localFileNameBase, m_localFileNameBase + ".mdl"))
		{
			SCModelDatabaseInternal()->BuildQueryModelResource(m_repoId, m_localFileNameBase, m_localFileNameBase + ".mdl", m_networkFileNameBase, m_networkFileNameBase + ".mdl", bHasTModel);
		}

		if (!m_networkFileNameBase.empty() && bHasTModel &&
			!SCModelDatabaseInternal()->IsModelResourceFileAvailable(m_localFileNameBase, m_localFileNameBase + "T.mdl") &&
			!SCModelDatabaseInternal()->IsModelResourceFileAvailable(m_localFileNameBase, m_localFileNameBase + "t.mdl"))
		{
			SCModelDatabaseInternal()->BuildQueryModelResource(m_repoId, m_localFileNameBase, m_localFileNameBase + "T.mdl", m_networkFileNameBase, m_networkFileNameBase + "t.mdl", bHasTModel);
			SCModelDatabaseInternal()->BuildQueryModelResource(m_repoId, m_localFileNameBase, m_localFileNameBase + "T.mdl", m_networkFileNameBase, m_networkFileNameBase + "T.mdl", bHasTModel);
		}

		if (!m_networkFileNameBase.empty() &&
			!SCModelDatabaseInternal()->IsModelResourceFileAvailable(m_localFileNameBase, m_localFileNameBase + ".bmp"))
		{
			SCModelDatabaseInternal()->BuildQueryModelResource(m_repoId, m_localFileNameBase, m_localFileNameBase + ".bmp", m_networkFileNameBase, m_networkFileNameBase + ".bmp", bHasTModel);
		}

		return true;
	}

	const char* GetName() const override
	{
		return "QueryTaskList";
	}

	const char* GetIdentifier() const override
	{
		return m_lowerName.c_str();
	}
};

class CSCModelQueryVersions : public CSCModelQueryBase
{
private:
	std::stringstream m_payloadstream;
	size_t m_receivedFileSize{};
	size_t m_expectedFileSize{};

public:
	CSCModelQueryVersions() : CSCModelQueryBase()
	{

	}

	const char* GetName() const override
	{
		return "QueryVersions";
	}

	const char* GetIdentifier() const override
	{
		return "versions.json";
	}

	void StartQuery() override
	{
		CSCModelQueryBase::StartQuery();

		m_Url = "https://raw.githubusercontent.com/wootguy/scmodels/master/database/sc/versions.json";

		if (SCModel_CDN() == 1)
		{
			m_Url = "https://cdn.jsdelivr.net/gh/wootguy/scmodels@master/database/sc/versions.json";
		}

		auto pRequestInstance = UtilHTTPClient()->CreateAsyncStreamRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));

		if (!pRequestInstance)
		{
			CSCModelQueryBase::OnFailure();
			return;
		}

		m_receivedFileSize = 0;

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();

		SCModelDatabaseInternal()->DispatchQueryStateChangeCallback(this, GetState());
	}

	float GetProgress() const override
	{
		if (m_expectedFileSize > 0)
			return (float)m_receivedFileSize / (float)m_expectedFileSize;

		return -1;
	}

	void OnResponding(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		CSCModelQueryBase::OnResponding(RequestInstance, ResponseInstance);

		m_receivedFileSize = 0;

		m_payloadstream = std::stringstream();
	}

	bool OnStreamComplete(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		bool bSuccess = false;

		if (1)
		{
			auto payload = m_payloadstream.str();

			gEngfuncs.Con_Printf("[SCModelDownloader] versions.json downloaded.\n");

			auto fileSize = payload.size();

			if (m_expectedFileSize >= 0 && fileSize < m_expectedFileSize)
			{
				gEngfuncs.Con_Printf("[SCModelDownloader] versions.json size mismatch ! expect %d, got %d. The downloading progress might be interrupted.\n", m_expectedFileSize, fileSize);
				return false;
			}

			bSuccess = SCModelDatabaseInternal()->OnVersionsJSONAcquired(payload.c_str(), payload.size(), "versions.json.tmp");

			if (bSuccess)
			{
				std::string filePath = "scmodeldownloader/versions.json";

				auto hFileHandleWrite = FILESYSTEM_ANY_OPEN(filePath.c_str(), "wb", "GAME");

				if (hFileHandleWrite)
				{
					SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(hFileHandleWrite); };

					FILESYSTEM_ANY_WRITE(payload.c_str(), payload.size(), hFileHandleWrite);
				}
				else
				{
					gEngfuncs.Con_Printf("[SCModelDownloader] Failed to open file \"%s\" for writing!\n", filePath.c_str());
				}
			}
		}

		return bSuccess;
	}

	void OnReceiveChunk(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		if (!m_expectedFileSize)
		{
			auto nContentLength = UTIL_GetContentLength(ResponseInstance);

			if (nContentLength > 0)
				m_expectedFileSize = nContentLength;
		}

		m_payloadstream.write((const char*)data, size);

		m_receivedFileSize += size;

		SCModelDatabaseInternal()->DispatchQueryStateChangeCallback(this, SCModelQueryState_Receiving);
	}
};

class CSCModelQueryDatabase : public CSCModelQueryBase
{
private:
	std::stringstream m_payloadstream;
	size_t m_receivedFileSize{};
	size_t m_expectedFileSize{};

public:
	CSCModelQueryDatabase() : CSCModelQueryBase()
	{

	}

	const char* GetName() const override
	{
		return "QueryDatabase";
	}

	const char* GetIdentifier() const override
	{
		return "models.json";
	}

	void StartQuery() override
	{
		CSCModelQueryBase::StartQuery();

		m_Url = "https://raw.githubusercontent.com/wootguy/scmodels/master/database/sc/models.json";

		if (SCModel_CDN() == 1)
		{
			m_Url = "https://cdn.jsdelivr.net/gh/wootguy/scmodels@master/database/sc/models.json";
		}

		auto pRequestInstance = UtilHTTPClient()->CreateAsyncStreamRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));

		if (!pRequestInstance)
		{
			CSCModelQueryBase::OnFailure();
			return;
		}

		m_receivedFileSize = 0;

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();

		SCModelDatabaseInternal()->DispatchQueryStateChangeCallback(this, GetState());
	}

	float GetProgress() const override
	{
		if (m_expectedFileSize > 0)
			return (float)m_receivedFileSize / (float)m_expectedFileSize;

		return -1;
	}

	void OnResponding(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		CSCModelQueryBase::OnResponding(RequestInstance, ResponseInstance);

		m_receivedFileSize = 0;

		m_payloadstream = std::stringstream();
	}

	bool OnStreamComplete(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		bool bSuccess = false;

		if (1)
		{
			auto payload = m_payloadstream.str();

			gEngfuncs.Con_Printf("[SCModelDownloader] models.json downloaded.\n");

			auto fileSize = payload.size();

			if (m_expectedFileSize >= 0 && fileSize < m_expectedFileSize)
			{
				gEngfuncs.Con_Printf("[SCModelDownloader] models.json size mismatch ! expect %d, got %d. The downloading progress might be interrupted.\n", m_expectedFileSize, fileSize);
				return false;
			}

			bSuccess = SCModelDatabaseInternal()->OnDatabaseJSONAcquired(payload.c_str(), payload.size(), "models.json.tmp");

			if (bSuccess)
			{
				std::string filePath = "scmodeldownloader/models.json";

				auto hFileHandleWrite = FILESYSTEM_ANY_OPEN(filePath.c_str(), "wb", "GAME");

				if (hFileHandleWrite)
				{
					SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(hFileHandleWrite); };

					FILESYSTEM_ANY_WRITE(payload.c_str(), payload.size(), hFileHandleWrite);
				}
				else
				{
					gEngfuncs.Con_Printf("[SCModelDownloader] Failed to open file \"%s\" for writing!\n", filePath.c_str());
				}
			}
		}

		return bSuccess;
	}

	void OnReceiveChunk(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		if (!m_expectedFileSize)
		{
			auto nContentLength = UTIL_GetContentLength(ResponseInstance);

			if (nContentLength > 0)
				m_expectedFileSize = nContentLength;
		}

		m_payloadstream.write((const char *)data, size);

		m_receivedFileSize += size;

		SCModelDatabaseInternal()->DispatchQueryStateChangeCallback(this, SCModelQueryState_Receiving);
	}
};

class CSCModelDatabase : public ISCModelDatabaseInternal
{
private:
	/*
		Purpose: The container that keeps strong refs to all queries.
	*/
	std::vector<UtilAutoPtr<ISCModelQueryInternal>> m_QueryList;
	
	/*
		Purpose: Dispatch callbacks when a query object has changed it's state.
	*/
	std::vector<ISCModelQueryStateChangeHandler*> m_StateChangeCallbacks;

	/*
		Purpose: Dispatch callbacks when local player has changed to a new model.
	*/
	std::vector<ISCModelLocalPlayerModelChangeHandler*> m_LocalPlayerChangeModelCallbacks;

	/*
		Purpose: Skippped model when player click cancel in OnLocalPlayerChangeModel->SwitchtoNewerVersionModel QueryBox.
	*/
	std::vector<std::string> m_SkippedModels;

	/*
		Purpose: Map model name to scmodel_t (always lower case)
	*/
	std::unordered_map<std::string, scmodel_t> m_Database;
	
	/*
		Purpose: Map older version to it's latest version. i.e. "gfl_m14-c2" maps to "gfl_m14-c2_v2" (always lower case)
	*/
	std::unordered_map<std::string, std::string> m_VersionMapping;

	/*
		Purpose: Store last model name that local player was using. so we can emit a model change event by checking if local player's model name differs from m_localPlayerModelName
	*/
	std::string m_localPlayerModelName;

public:

	void Init() override
	{
		if (!LoadLocalDatabase("scmodeldownloader/models.json"))
		{
			BuildQueryDatabase();			
		}

		if (!LoadLocalVersions("scmodeldownloader/versions.json"))
		{
			BuildQueryVersions();			
		}
		
		// Load skipped models list
		LoadSkippedModels("scmodeldownloader/skippedmodels.txt");
	}

	void Shutdown() override
	{
		m_QueryList.clear();
		m_StateChangeCallbacks.clear();
		m_LocalPlayerChangeModelCallbacks.clear();
		m_Database.clear();
		m_VersionMapping.clear();
	}

	void RunFrame() override
	{
		auto flCurrentAbsTime = (float)gEngfuncs.GetAbsoluteTime();

		for (auto itor = m_QueryList.begin(); itor != m_QueryList.end();)
		{
			const auto& p = (*itor);

			p->RunFrame(flCurrentAbsTime);

			if (p->IsFinished() && !p->NeedRetry())
			{
				itor = m_QueryList.erase(itor);
				continue;
			}

			itor++;
		}

		auto model = gEngfuncs.LocalPlayerInfo_ValueForKey("model");
		if (model)
		{
			if (0 != stricmp(m_localPlayerModelName.c_str(), model))
			{
				std::string previousModelName = m_localPlayerModelName;
				m_localPlayerModelName = model;
				std::transform(m_localPlayerModelName.begin(), m_localPlayerModelName.end(), m_localPlayerModelName.begin(), ::tolower);

				DispatchLocalPlayerChangeModelCallback(previousModelName, m_localPlayerModelName);
			}
		}
	}

	void BuildQueryModelResource(
		int repoId,
		const std::string& localFileNameBase,
		const std::string& localFileName,
		const std::string& networkFileNameBase,
		const std::string& networkFileName,
		bool bHasTModel) override
	{
		auto identifier = std::format("/{0}", networkFileName);

		for (const auto& p : m_QueryList)
		{
			if (!strcmp(p->GetName(), "QueryModelResource") &&
				!strcmp(p->GetIdentifier(), identifier.c_str()))
			{
				return;
			}
		}

		auto pQuery = new CSCModelQueryModelResourceTask(repoId, localFileNameBase, localFileName, networkFileNameBase, networkFileName, bHasTModel);

		m_QueryList.emplace_back(pQuery);

		pQuery->StartQuery();

		pQuery->Release();

		gEngfuncs.Con_Printf("[SCModelDownloader] Downloading \"%s\"...\n", identifier.c_str());
	}

	/*
		Purpose: Build query instance, to get detailed information about model with name as "lowerName", in json.
	*/

	bool BuildQueryListInternal(const std::string& lowerName, const std::string& localFileNameBase)
	{
		for (const auto& p : m_QueryList)
		{
			if (!strcmp(p->GetName(), "QueryTaskList") &&
				!strcmp(p->GetIdentifier(), lowerName.c_str()))
			{
				return false;
			}
		}

		auto pQuery = new CSCModelQueryTaskList(lowerName, localFileNameBase);

		m_QueryList.emplace_back(pQuery);

		pQuery->StartQuery();

		pQuery->Release();

		gEngfuncs.Con_Printf("[SCModelDownloader] Querying file list for \"%s\"...\n", lowerName.c_str());
		return true;
	}

	/*
		Purpose: Build query based on modelName, to get file list about this model.
		lowerName is always lower-case, while modelName is not
	*/

	bool BuildQueryList(const std::string& modelName)
	{
		std::string lowerName = modelName;

		std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

		//Model name ends with "_v0" ~ "_v9"
		if (lowerName.length() > 4 &&
			lowerName[lowerName.length() - 3] == '_' &&
			lowerName[lowerName.length() - 2] == 'v' &&
			lowerName[lowerName.length() - 1] >= '0' &&
			lowerName[lowerName.length() - 1] <= '9')
		{
			//has version?
			auto itor = m_Database.find(lowerName);
			if (itor != m_Database.end())
			{
				return BuildQueryListInternal(lowerName, modelName);
			}
			else
			{
				gEngfuncs.Con_DPrintf("[SCModelDownloader] \"%s\" not found in database.\n", modelName.c_str());
				return false;
			}
		}

		//Try latest if no version was found
		if (SCModel_ShouldDownloadLatest())
		{
#if 1
			// Check if there is a version mapping for this model name
			auto versionMappingItor = m_VersionMapping.find(lowerName);
			if (versionMappingItor != m_VersionMapping.end())
			{
				const std::string& latestVersion = versionMappingItor->second;

				// Check if the latest version exists in the database
				auto databaseItor = m_Database.find(latestVersion);
				if (databaseItor != m_Database.end())
				{
					return BuildQueryListInternal(latestVersion, modelName);
				}
			}
#else
			for (int i = 9; i >= 1; --i)
			{
				auto lowerName2 = lowerName;
				lowerName2 += "_v";
				lowerName2 += ('0' + i);

				auto itor = m_Database.find(lowerName2);

				if (itor != m_Database.end())
				{
					return BuildQueryListInternal(lowerName2, modelName);
				}
			}
#endif
		}

		return BuildQueryListInternal(lowerName, modelName);
	}

	bool BuildQueryVersions() override
	{
		for (const auto& p : m_QueryList)
		{
			if (!strcmp(p->GetName(), "QueryVersions"))
			{
				return false;
			}
		}

		auto pQuery = new CSCModelQueryVersions();

		m_QueryList.emplace_back(pQuery);

		pQuery->StartQuery();

		pQuery->Release();

		gEngfuncs.Con_DPrintf("[SCModelDownloader] Querying versions.json ...\n");

		return true;
	}

	bool BuildQueryDatabase() override
	{
		for (const auto& p : m_QueryList)
		{
			if (!strcmp(p->GetName(), "QueryDatabase"))
			{
				return false;
			}
		}

		auto pQuery = new CSCModelQueryDatabase();

		m_QueryList.emplace_back(pQuery);

		pQuery->StartQuery();

		pQuery->Release();

		gEngfuncs.Con_DPrintf("[SCModelDownloader] Querying models.json ...\n");

		return true;
	}

	void QueryModel(const char* name) override
	{
		if (IsAllRequiredFilesForModelAvailable(name, false))
		{
			gEngfuncs.Con_DPrintf("[SCModelDownloader] Model \"%s\" already available, query ignored.\n", name);
			return;
		}

		gEngfuncs.Con_Printf("[SCModelDownloader] Querying model json for \"%s\"...\n", name);

		//What if database isn't available yet ?

		BuildQueryList(name);
	}

	void EnumQueries(IEnumSCModelQueryHandler* handler) override
	{
		for (const auto& p : m_QueryList)
		{
			handler->OnEnumQuery(p);
		}
	}

	void RegisterQueryStateChangeCallback(ISCModelQueryStateChangeHandler* handler) override
	{
		auto itor = std::find_if(m_StateChangeCallbacks.begin(), m_StateChangeCallbacks.end(), [handler](ISCModelQueryStateChangeHandler* it) {
			return it == handler;
			});

		if (itor == m_StateChangeCallbacks.end())
		{
			m_StateChangeCallbacks.emplace_back(handler);
		}
	}

	void UnregisterQueryStateChangeCallback(ISCModelQueryStateChangeHandler* handler) override
	{
		auto itor = std::find_if(m_StateChangeCallbacks.begin(), m_StateChangeCallbacks.end(), [handler](ISCModelQueryStateChangeHandler* it) {
			return it == handler;
			});

		if (itor != m_StateChangeCallbacks.end())
		{
			m_StateChangeCallbacks.erase(itor);
		}
	}

	void RegisterLocalPlayerChangeModelCallback(ISCModelLocalPlayerModelChangeHandler* handler) override
	{
		auto itor = std::find_if(m_LocalPlayerChangeModelCallbacks.begin(), m_LocalPlayerChangeModelCallbacks.end(), [handler](ISCModelLocalPlayerModelChangeHandler* it) {
			return it == handler;
			});

		if (itor == m_LocalPlayerChangeModelCallbacks.end())
		{
			m_LocalPlayerChangeModelCallbacks.emplace_back(handler);
		}
	}

	void UnregisterLocalPlayerChangeModelCallback(ISCModelLocalPlayerModelChangeHandler* handler) override
	{
		auto itor = std::find_if(m_LocalPlayerChangeModelCallbacks.begin(), m_LocalPlayerChangeModelCallbacks.end(), [handler](ISCModelLocalPlayerModelChangeHandler* it) {
			return it == handler;
			});

		if (itor != m_LocalPlayerChangeModelCallbacks.end())
		{
			m_LocalPlayerChangeModelCallbacks.erase(itor);
		}
	}

	void DispatchQueryStateChangeCallback(ISCModelQuery* pQuery, SCModelQueryState newState) override
	{
		for (auto callback : m_StateChangeCallbacks)
		{
			callback->OnQueryStateChanged(pQuery, newState);
		}
	}

	void DispatchLocalPlayerChangeModelCallback(const std::string& previousModelName, const std::string& newModelName)
	{
		for (auto callback : m_LocalPlayerChangeModelCallbacks)
		{
			callback->OnLocalPlayerChangeModel(previousModelName.c_str(), newModelName.c_str());
		}
	}

	bool IsModelResourceFileAvailable(const std::string& localFileNameBase, const std::string& localFileName) override
	{
		std::string filePath = std::format("models/player/{0}/{1}", localFileNameBase, localFileName);

		if (!FILESYSTEM_ANY_FILEEXISTS(filePath.c_str()))
			return false;

		return true;
	}

	bool IsAllRequiredFilesForModelAvailable(const std::string& localFileNameBase, bool bHasTModel) override
	{
		if (!IsModelResourceFileAvailable(localFileNameBase, localFileNameBase + ".mdl"))
			return false;

		if (bHasTModel)
		{
			//Linux is unhappy with t.mdl
			if (!IsModelResourceFileAvailable(localFileNameBase, localFileNameBase + "T.mdl") &&
				!IsModelResourceFileAvailable(localFileNameBase, localFileNameBase + "t.mdl"))
			{
				return false;
			}
		}

		return true;
	}

	bool IsAllRequiredFilesForModelAvailableCABI(const char * localFileNameBase, bool bHasTModel) override
	{
		return IsAllRequiredFilesForModelAvailable(std::string(localFileNameBase), bHasTModel);
	}

	void OnModelFileWriteFinished(const std::string& localFileNameBase, bool bHasTModel) override
	{
		if (IsAllRequiredFilesForModelAvailable(localFileNameBase, bHasTModel))
		{
			SCModel_ReloadModel(localFileNameBase.c_str());
		}
	}

	bool LoadLocalVersions(const char* filePath)
	{
		auto hFileHandle = FILESYSTEM_ANY_OPEN(filePath, "rb");

		if (hFileHandle)
		{
			SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(hFileHandle); };

			auto fileSize = FILESYSTEM_ANY_SIZE(hFileHandle);

			if (fileSize > 0)
			{
				auto fileBuf = malloc(fileSize);

				if (fileBuf)
				{
					SCOPE_EXIT{ free(fileBuf); };

					if (FILESYSTEM_ANY_READ(fileBuf, fileSize, hFileHandle) == fileSize)
					{
						return OnVersionsJSONAcquired((const char*)fileBuf, fileSize, filePath);
					}
					else
					{
						gEngfuncs.Con_Printf("[SCModelDownloader] Failed to read content from local \"scmodeldownloader/versions.json\".\n");
					}
				}
				else
				{
					gEngfuncs.Con_Printf("[SCModelDownloader] Failed to allocate %d bytes for \"scmodeldownloader/versions.json\".\n", fileSize);
				}
			}
			else
			{
				gEngfuncs.Con_Printf("[SCModelDownloader] Local database \"scmodeldownloader/versions.json\" is empty.\n");
			}
		}
		else
		{
			gEngfuncs.Con_Printf("[SCModelDownloader] Local database \"scmodeldownloader/versions.json\" is not available.\n");
		}

		return false;
	}

	bool OnVersionsJSONAcquired(const char* data, size_t size, const char* sourceFile) override
	{
		if (size == 0)
		{
			gEngfuncs.Con_Printf("[SCModelDownloader] Empty versions acquired!\n");
			return false;
		}

		rapidjson::Document doc;

		doc.Parse(data, size);

		if (doc.HasParseError())
		{
			gEngfuncs.Con_Printf("[SCModelDownloader] Failed to parse versions json! errorcode = %d, error_offset = %d, payload_size = %d.\n", doc.GetParseError(), doc.GetErrorOffset(), size);
			return false;
		}

		gEngfuncs.Con_Printf("[SCModelDownloader] versions.json acquired!\n");

		m_VersionMapping.clear();

		// Parse the JSON array structure as described in the comments
		// The JSON structure is an array of arrays, where:
		// - The first string element is always the latest version name
		// - The later string elements are the source model names that should map to the latest version
		if (doc.IsArray())
		{
			for (auto& versionArray : doc.GetArray())
			{
				if (versionArray.IsArray())
				{
					const auto& versionEntries = versionArray.GetArray();
					
					if (versionEntries.Size() >= 2)
					{
						// The first element is the latest version name
						std::string latestVersion = versionEntries[0].GetString();
						
						// Map all the source model names to the latest version
						for (rapidjson::SizeType i = 1; i < versionEntries.Size(); ++i)
						{
							if (versionEntries[i].IsString())
							{
								std::string sourceModelName = versionEntries[i].GetString();
								m_VersionMapping[sourceModelName] = latestVersion;
							}
						}
					}
				}
			}
		}

		return true;
	}

	bool LoadLocalDatabase(const char *filePath)
	{
		auto hFileHandle = FILESYSTEM_ANY_OPEN(filePath, "rb");

		if (hFileHandle)
		{
			SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(hFileHandle); };

			auto fileSize = FILESYSTEM_ANY_SIZE(hFileHandle);

			if (fileSize > 0)
			{
				auto fileBuf = malloc(fileSize);

				if (fileBuf)
				{
					SCOPE_EXIT{ free(fileBuf); };

					if (FILESYSTEM_ANY_READ(fileBuf, fileSize, hFileHandle) == fileSize)
					{
						return OnDatabaseJSONAcquired((const char*)fileBuf, fileSize, filePath);
					}
					else
					{
						gEngfuncs.Con_Printf("[SCModelDownloader] Failed to read content from local \"scmodeldownloader/models.json\".\n");
					}
				}
				else
				{
					gEngfuncs.Con_Printf("[SCModelDownloader] Failed to allocate %d bytes for \"scmodeldownloader/models.json\".\n", fileSize);
				}
			}
			else
			{
				gEngfuncs.Con_Printf("[SCModelDownloader] Local database \"scmodeldownloader/models.json\" is empty.\n");
			}
		}
		else
		{
			gEngfuncs.Con_Printf("[SCModelDownloader] Local database \"scmodeldownloader/models.json\" is not available.\n");
		}

		return false;
	}

	bool OnDatabaseJSONAcquired(const char* data, size_t size, const char * sourceFile) override
	{
		if (size == 0)
		{
			gEngfuncs.Con_Printf("[SCModelDownloader] Empty database acquired!\n");
			return false;
		}

		rapidjson::Document doc;

		doc.Parse(data, size);

		if (doc.HasParseError())
		{
			gEngfuncs.Con_Printf("[SCModelDownloader] Failed to parse database json! errorcode = %d, error_offset = %d, payload_size = %d.\n", doc.GetParseError(), doc.GetErrorOffset(), size);
			return false;
		}

		gEngfuncs.Con_Printf("[SCModelDownloader] models.json acquired!\n");

		m_Database.clear();

		for (auto itor = doc.MemberBegin(); itor != doc.MemberEnd(); ++itor)
		{
			std::string name = itor->name.GetString();

			const auto& obj = itor->value.GetObj();

			scmodel_t m = { 0 };

			const auto& m_size = obj.FindMember("size");
			if (m_size != obj.MemberEnd() && m_size->value.IsInt())
			{
				m.size = m_size->value.GetInt();
			}

			const auto& m_flags = obj.FindMember("flags");
			if (m_flags != obj.MemberEnd() && m_flags->value.IsInt())
			{
				m.flags = m_flags->value.GetInt();
			}

			const auto& m_polys = obj.FindMember("polys");
			if (m_polys != obj.MemberEnd() && m_polys->value.IsInt())
			{
				m.polys = m_polys->value.GetInt();
			}

			m_Database[name] = m;
		}

		return true;
	}

	const char* GetNewerVersionModel(const char* modelname) override
	{
		std::string lowerModelName = modelname;
		std::transform(lowerModelName.begin(), lowerModelName.end(), lowerModelName.begin(), ::tolower);

		// Check if there is a version mapping for this model name
		auto versionMappingItor = m_VersionMapping.find(lowerModelName);
		if (versionMappingItor != m_VersionMapping.end())
		{
			const std::string& latestVersion = versionMappingItor->second;

			// Check if the latest version exists in the database
			auto databaseItor = m_Database.find(latestVersion);
			if (databaseItor != m_Database.end())
			{
				return latestVersion.c_str();
			}
		}
		return nullptr;
	}

	/*
		Purpose: Check if modelname has been skipped by user
	*/
	bool IsModelSkipped(const char* modelname) override
	{
		std::string lowerModelName = modelname;
		std::transform(lowerModelName.begin(), lowerModelName.end(), lowerModelName.begin(), ::tolower);
		
		auto itor = std::find(m_SkippedModels.begin(), m_SkippedModels.end(), lowerModelName);
		return itor != m_SkippedModels.end();
	}

	/*
		Purpose: Add modelname to skipped models and save to file "scmodeldatabase/skippedmodels.txt".
	*/
	void AddSkippedModel(const char* modelname) override
	{
		std::string lowerModelName = modelname;
		std::transform(lowerModelName.begin(), lowerModelName.end(), lowerModelName.begin(), ::tolower);
		
		// Check if already exists
		auto itor = std::find(m_SkippedModels.begin(), m_SkippedModels.end(), lowerModelName);
		if (itor == m_SkippedModels.end())
		{
			m_SkippedModels.emplace_back(lowerModelName);
			SaveSkippedModels("scmodeldownloader/skippedmodels.txt");
		}
	}

	/*
		Purpose: Load skipped models from filePath, one model name per line.
	*/
	void LoadSkippedModels(const char *filePath)
	{
		auto hFileHandle = FILESYSTEM_ANY_OPEN(filePath, "rt");
		if (hFileHandle)
		{
			m_SkippedModels.clear();
			
			char szReadLine[256];
			while (!FILESYSTEM_ANY_EOF(hFileHandle))
			{
				FILESYSTEM_ANY_READLINE(szReadLine, sizeof(szReadLine) - 1, hFileHandle);
				szReadLine[sizeof(szReadLine) - 1] = 0;
				
				// Parse line and extract model name
				bool quoted = false;
				char token[256];
				char *p = szReadLine;
				
				p = FILESYSTEM_ANY_PARSEFILE(p, token, &quoted);
				if (token[0])
				{
					std::string modelName = token;
					std::transform(modelName.begin(), modelName.end(), modelName.begin(), ::tolower);
					
					// Avoid duplicates
					auto itor = std::find(m_SkippedModels.begin(), m_SkippedModels.end(), modelName);
					if (itor == m_SkippedModels.end())
					{
						m_SkippedModels.emplace_back(modelName);
					}
				}
			}
			
			FILESYSTEM_ANY_CLOSE(hFileHandle);
			
			gEngfuncs.Con_DPrintf("[SCModelDownloader] Loaded %d skipped models from \"%s\".\n", m_SkippedModels.size(), filePath);
		}
		else
		{
			gEngfuncs.Con_DPrintf("[SCModelDownloader] Skipped models file \"%s\" not found.\n", filePath);
		}
	}

	/*
		Purpose: Save skipped models to filePath, one model name per line.
	*/
	void SaveSkippedModels(const char *filePath)
	{
		// Create directory if not exists
		FILESYSTEM_ANY_CREATEDIR("scmodeldownloader", "GAME");
		
		auto hFileHandle = FILESYSTEM_ANY_OPEN(filePath, "wt", "GAME");
		if (hFileHandle)
		{
			for (const auto& modelName : m_SkippedModels)
			{
				FILESYSTEM_ANY_WRITE(modelName.c_str(), modelName.length(), hFileHandle);
				FILESYSTEM_ANY_WRITE("\n", 1, hFileHandle);
			}
			
			FILESYSTEM_ANY_CLOSE(hFileHandle);
			
			gEngfuncs.Con_DPrintf("[SCModelDownloader] Saved %d skipped models to \"%s\".\n", m_SkippedModels.size(), filePath);
		}
		else
		{
			gEngfuncs.Con_Printf("[SCModelDownloader] Failed to open file \"%s\" for writing skipped models!\n", filePath);
		}
	}
};


static CSCModelDatabase s_SCModelDatabase;

ISCModelDatabase* SCModelDatabase()
{
	return &s_SCModelDatabase;
}

ISCModelDatabaseInternal* SCModelDatabaseInternal()
{
	return &s_SCModelDatabase;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSCModelDatabase, ISCModelDatabase, SCMODEL_DATABASE_INTERFACE_VERSION, s_SCModelDatabase)