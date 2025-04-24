#include <metahook.h>
#include "plugins.h"

#include "UtilHTTPClient.h"
#include "UtilAssetsIntegrity.h"
#include "SCModelDatabase.h"

#include <string>
#include <unordered_map>
#include <algorithm>
#include <format>

#include <rapidjson/document.h>
#include <ScopeExit/ScopeExit.h>

bool SCModel_ShouldDownloadLatest();
void SCModel_ReloadModel(const char* name);

static unsigned int g_uiAllocatedTaskId = 0;

class ISCModelQueryInternal : public ISCModelQuery
{
public:
	virtual void AddRef() = 0;
	virtual void Release() = 0;

	virtual void OnResponding(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) = 0;
	virtual void OnFinish(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) = 0;
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

	virtual bool OnDatabaseJSONAcquired(const char* data, size_t size) = 0;

	virtual void DispatchQueryStateChangeCallback(ISCModelQuery* pQuery, SCModelQueryState newState) = 0;
};

ISCModelDatabaseInternal* SCModelDatabaseInternal();

template<typename T>
class AutoPtr {
private:
	T* m_ptr;

public:
	AutoPtr() : m_ptr(nullptr) {}

	explicit AutoPtr(T* ptr) : m_ptr(ptr) {
		if (m_ptr) {
			m_ptr->AddRef();
		}
	}

	AutoPtr(const AutoPtr& other) : m_ptr(other.m_ptr) {
		if (m_ptr) {
			m_ptr->AddRef();
		}
	}

	AutoPtr(AutoPtr&& other) noexcept : m_ptr(other.m_ptr) {
		other.m_ptr = nullptr;
	}

	~AutoPtr() {
		if (m_ptr) {
			m_ptr->Release();
		}
	}

	AutoPtr& operator=(const AutoPtr& other) {
		if (this != &other) {
			if (m_ptr) {
				m_ptr->Release();
			}
			m_ptr = other.m_ptr;
			if (m_ptr) {
				m_ptr->AddRef();
			}
		}
		return *this;
	}

	AutoPtr& operator=(AutoPtr&& other) noexcept {
		if (this != &other) {
			if (m_ptr) {
				m_ptr->Release();
			}
			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}
		return *this;
	}

	T* operator->() const { return m_ptr; }
	T& operator*() const { return *m_ptr; }
	operator T* () const { return m_ptr; }

	T* Get() const { return m_ptr; }

	void Reset(T* ptr = nullptr) {
		if (m_ptr) {
			m_ptr->Release();
		}
		m_ptr = ptr;
		if (m_ptr) {
			m_ptr->AddRef();
		}
	}

	T* Detach() {
		T* ptr = m_ptr;
		m_ptr = nullptr;
		return ptr;
	}
};

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

		auto pPayload = ResponseInstance->GetPayload();

		if (!m_pQueryTask->OnProcessPayload(RequestInstance, ResponseInstance, (const void*)pPayload->GetBytes(), pPayload->GetLength()))
		{
			m_pQueryTask->OnFailure();
			return;
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

	void AddRef() override {
		InterlockedIncrement(&m_RefCount);
	}

	void Release() override {
		if (InterlockedDecrement(&m_RefCount) == 0) {
			delete this;
		}
	}

	const char* GetUrl() const override
	{
		return m_Url.c_str();
	}

	bool IsFinished() const override
	{
		return m_bFinished;
	}

	bool IsFailed() const override
	{
		return m_bFailed;
	}

	SCModelQueryState GetState() const override
	{
		if (m_bResponding)
			return SCModelQueryState_Receiving;

		if (m_bFailed)
			return SCModelQueryState_Failed;

		if (m_bFinished)
			return SCModelQueryState_Finished;

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

		m_bFailed = false;
		m_bFinished = false;
		SCModelDatabaseInternal()->DispatchQueryStateChangeCallback(this, GetState());
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
		m_identifier = std::format("/{0}", m_networkFileName);
	}

	~CSCModelQueryModelResourceTask()
	{
		if (m_hFileHandle)
		{
			FILESYSTEM_ANY_CLOSE(m_hFileHandle);
			m_hFileHandle = nullptr;
		}
	}

	void StartQuery() override
	{
		CSCModelQueryBase::StartQuery();

		m_Url = std::format("https://wootdata.github.io/scmodels_data_{0}/models/player/{1}/{2}", m_repoId, m_networkFileNameBase, m_networkFileName);

		auto pRequestInstance = UtilHTTPClient()->CreateAsyncStreamRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));

		if (!pRequestInstance)
		{
			CSCModelQueryBase::OnFailure();
			return;
		}

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();
	}

	void OnResponding(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		CSCModelQueryBase::OnResponding(RequestInstance, ResponseInstance);

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

	void OnFinish(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		CSCModelQueryBase::OnFinish(RequestInstance, ResponseInstance);

		auto nContentLength = UTIL_GetContentLength(ResponseInstance);

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
					if (nContentLength >= 0 && fileSize < nContentLength)
					{
						gEngfuncs.Con_Printf("[SCModelDownloader] Temp file \"%s\" size mismatch ! expect %d, got %d. The downloading progress might be interrupted.\n", filePathTmp.c_str(), nContentLength, fileSize);
						return;
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
									return;
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

								if (UtilAssetsIntegrityCheckReason::OK != UtilAssetsIntegrity()->Check8bitBMP(fileBuf, fileSize, &checkResult))
								{
									gEngfuncs.Con_Printf("[SCModelDownloader] File \"%s\" failed the integrity check!\n", filePathTmp.c_str());
									gEngfuncs.Con_Printf("[SCModelDownloader] Integrity check result: %s.\n", checkResult.ReasonStr);
									return;
								}
							}
							std::string filePath = std::format("models/player/{0}/{1}", m_localFileNameBase, m_localFileName);

							auto hFileHandleWrite = FILESYSTEM_ANY_OPEN(filePath.c_str(), "wb", "GAMEDOWNLOAD");

							if (hFileHandleWrite)
							{
								SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(hFileHandleWrite); };

								FILESYSTEM_ANY_WRITE(fileBuf, fileSize, hFileHandleWrite);
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
	}

	void OnReceiveChunk(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		if (m_hFileHandle)
		{
			FILESYSTEM_ANY_WRITE(data, size, m_hFileHandle);
		}
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

	std::vector<std::shared_ptr<ISCModelQuery>> m_SubQueryList;

public:
	CSCModelQueryTaskList(const std::string& lowerName, const std::string& localFileNameBase) :
		CSCModelQueryBase(),
		m_repoId(SCModel_Hash(lowerName) % 32),
		m_lowerName(lowerName),
		m_localFileNameBase(localFileNameBase)
	{

	}

	bool IsFinished() const override
	{
		if (!CSCModelQueryBase::IsFinished())
			return false;

		for (auto& itor : m_SubQueryList)
		{
			if (!itor->IsFinished())
				return false;
		}

		return true;
	}

	void StartQuery() override
	{
		CSCModelQueryBase::StartQuery();

		m_Url = std::format("https://wootdata.github.io/scmodels_data_{0}/models/player/{1}/{1}.json", m_repoId, m_lowerName);

		auto pRequestInstance = UtilHTTPClient()->CreateAsyncRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));

		if (!pRequestInstance)
		{
			CSCModelQueryBase::OnFailure();
			return;
		}

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();
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

class CSCModelQueryDatabase : public CSCModelQueryBase
{
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
		return "";
	}

	void StartQuery() override
	{
		CSCModelQueryBase::StartQuery();

		m_Url = "https://raw.githubusercontent.com/wootguy/scmodels/master/database/models.json";

		auto pRequestInstance = UtilHTTPClient()->CreateAsyncRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));

		if (!pRequestInstance)
		{
			CSCModelQueryBase::OnFailure();
			return;
		}

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();
	}

	bool OnProcessPayload(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		if (!CSCModelQueryBase::OnProcessPayload(RequestInstance, ResponseInstance, data, size))
			return false;

		return SCModelDatabaseInternal()->OnDatabaseJSONAcquired((const char*)data, size);
	}
};

class CSCModelDatabase : public ISCModelDatabaseInternal
{
private:
	std::vector<AutoPtr<ISCModelQueryInternal>> m_QueryList;
	std::vector<ISCModelQueryStateChangeHandler*> m_StateChangeCallbacks;
	std::unordered_map<std::string, scmodel_t> m_Database;

public:

	void Init() override
	{
		LoadLocalDatabase();
		BuildQueryDatabase();
	}

	void Shutdown() override
	{
		m_QueryList.clear();
		m_StateChangeCallbacks.clear();
		m_Database.clear();
	}

	void RunFrame() override
	{
		auto flCurrentAbsTime = (float)gEngfuncs.GetAbsoluteTime();

		for (auto itor = m_QueryList.begin(); itor != m_QueryList.end();)
		{
			const auto& p = (*itor);

			p->RunFrame(flCurrentAbsTime);

			if (p->IsFinished())
			{
				itor = m_QueryList.erase(itor);
				continue;
			}

			itor++;
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
		}

		return BuildQueryListInternal(lowerName, modelName);;
	}

	bool BuildQueryDatabase()
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

		gEngfuncs.Con_DPrintf("[SCModelDownloader] Querying database...\n");

		return true;
	}

	void QueryModel(const char* name) override
	{
		gEngfuncs.Con_Printf("[SCModelDownloader] Querying model \"%s\"...\n", name);

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

	void DispatchQueryStateChangeCallback(ISCModelQuery* pQuery, SCModelQueryState newState) override
	{
		for (auto callback : m_StateChangeCallbacks)
		{
			callback->OnQueryStateChanged(pQuery, newState);
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

	void OnModelFileWriteFinished(const std::string& localFileNameBase, bool bHasTModel) override
	{
		if (IsAllRequiredFilesForModelAvailable(localFileNameBase, bHasTModel))
		{
			SCModel_ReloadModel(localFileNameBase.c_str());
		}
	}

	void LoadLocalDatabase()
	{
		auto hFileHandle = FILESYSTEM_ANY_OPEN("scmodeldownloader/models.json", "rb");

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
						OnDatabaseJSONAcquired((const char*)fileBuf, fileSize);
					}
					else
					{
						gEngfuncs.Con_Printf("[SCModelDownloader] LoadLocalDatabase: failed to read content from local \"scmodeldownloader/models.json\".");
					}
				}
				else
				{
					gEngfuncs.Con_Printf("[SCModelDownloader] LoadLocalDatabase: failed to allocate %d bytes for \"scmodeldownloader/models.json\".", fileSize);
				}
			}
			else
			{
				gEngfuncs.Con_Printf("[SCModelDownloader] LoadLocalDatabase: local \"scmodeldownloader/models.json\" is empty.");
			}
		}
		else
		{
			gEngfuncs.Con_Printf("[SCModelDownloader] LoadLocalDatabase: local \"scmodeldownloader/models.json\" is not available.");
		}
	}

	bool OnDatabaseJSONAcquired(const char* data, size_t size) override
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