#include <metahook.h>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <regex>
#include <strtools.h>
#include "ResourceReplacer.h"
#include "util.h"

class IResourceReplaceEntry
{
public:
	virtual ~IResourceReplaceEntry()
	{

	}

	/*
		Purpose: Fill ReplacedFileName with replaced file name if szSourceFileName matched.
	*/
	virtual bool GetReplacedFileName(const char* szSourceFileName, std::string &ReplacedFileName) = 0;
};

class CPlainResourceReplaceEntry : public IResourceReplaceEntry
{
public:
	CPlainResourceReplaceEntry(const char *src, const char* replace) : m_SourceFileName (src), m_ReplacedFileName(replace)
	{

	}
	CPlainResourceReplaceEntry(const std::string & src, const std::string& replace) : m_SourceFileName(src), m_ReplacedFileName(replace)
	{

	}
	~CPlainResourceReplaceEntry()
	{

	}

	bool GetReplacedFileName(const char* szSourceFileName, std::string &ReplacedFileName) override
	{
		if (!stricmp(m_SourceFileName.c_str(), szSourceFileName))
		{
			auto SourceExtension = V_GetFileExtension(m_SourceFileName.c_str());
			auto ReplaceExtension = V_GetFileExtension(m_ReplacedFileName.c_str());

			if (!stricmp(SourceExtension, ReplaceExtension))
			{
				ReplacedFileName = m_ReplacedFileName;
				return true;
			}
		}

		return false;
	}

private:
	std::string m_SourceFileName;
	std::string m_ReplacedFileName;
};

class CRegexResourceReplaceEntry : public IResourceReplaceEntry
{
public:
	CRegexResourceReplaceEntry(const std::string& pattern, const std::string& replace)
		: m_Pattern(pattern), m_Replacement(replace), m_Regex(pattern, std::regex::ECMAScript)
	{

	}

	bool GetReplacedFileName(const char* szSourceFileName, std::string& ReplacedFileName) override
	{
		std::cmatch match;
		if (std::regex_match(szSourceFileName, match, m_Regex))
		{
			std::string NewReplacedFileName = std::regex_replace(szSourceFileName, m_Regex, m_Replacement);

			auto SourceExtension = V_GetFileExtension(szSourceFileName);
			auto ReplaceExtension = V_GetFileExtension(NewReplacedFileName.c_str());

			if (!stricmp(SourceExtension, ReplaceExtension))
			{
				ReplacedFileName = NewReplacedFileName;
				return true;
			}
		}
		return false;
	}

private:
	std::string m_Pattern;
	std::string m_Replacement;
	std::regex m_Regex;
};

class CResourceReplacer : public IResourceReplacer
{
private:
	std::vector<IResourceReplaceEntry*> m_GlobalEntries;
	std::vector<IResourceReplaceEntry*> m_MapEntries;

public:

	void FreeMapEntries() override
	{
		for (auto entry : m_MapEntries)
		{
			delete entry;
		}

		m_MapEntries.clear();
	}

	void FreeGlobalEntries() override
	{
		for (auto entry : m_GlobalEntries)
		{
			delete entry;
		}

		m_GlobalEntries.clear();
	}

	void Shutdown() override
	{
		FreeMapEntries();
		FreeGlobalEntries();
	}

	/*
	Purpose: Parse replace list like, the quota can be omitted.
			Ignore lines start with ## or //
	
	## Pipe Wrench
	"models/p_pipe_wrench.mdl" "models/not_precached.mdl"
	"models/v_pipe_wrench.mdl" "models/not_precached.mdl"
	"models/w_pipe_wrench.mdl" "models/not_precached.mdl"

	// Medkit
	models/p_medkit.mdl models/not_precached.mdl
	models/v_medkit.mdl models/not_precached.mdl
	models/w_pmedkit.mdl models/not_precached.mdl

	// use std::regex to match and replace filename if regex is specified
	"models/aaa/(.*)\.mdl" "models/bbb/$1.mdl" regex

	*/

	void LoadReplaceList(const char* pszFileContent, std::vector<IResourceReplaceEntry*> &entries)
	{
		std::istringstream fileStream(pszFileContent); // Use istringstream to read the file content line by line
		std::string line;

		while (std::getline(fileStream, line))
		{
			// Trim leading and trailing whitespace
			line = TrimString(line);

			// Skip empty lines and comments
			if (line.empty() || line[0] == '#' || line[0] == '/')
				continue;

			// Split the line into two tokens
			std::istringstream lineStream(line);
			std::string src, replace, token;
			lineStream >> std::quoted(src) >> std::quoted(replace); // std::quoted to handle optional quotes around file names

			// Check if both file names were read successfully
			if (!src.empty() && !replace.empty())
			{
				if (lineStream >> token && token == "regex")
				{
					// If the token "regex" is found, use the regex-based replacement entry
					CRegexResourceReplaceEntry* entry = new CRegexResourceReplaceEntry(src, replace);
					entries.push_back(entry);
				}
				else
				{
					// Otherwise, use the plain replacement entry
					CPlainResourceReplaceEntry* entry = new CPlainResourceReplaceEntry(src, replace);
					entries.push_back(entry);
				}
			}
		}
	}

	void LoadGlobalReplaceList(const char *szFileName) override
	{
		const char* pszFileContent = (const char*)gEngfuncs.COM_LoadFile(szFileName, 5, NULL);

		if (!pszFileContent)
		{
			gEngfuncs.Con_DPrintf("LoadGlobalReplaceList: Could not load replace list file \"%s\"\n", szFileName);
			return;
		}

		LoadReplaceList(pszFileContent, m_GlobalEntries);

		gEngfuncs.COM_FreeFile((void*)pszFileContent);
	}

	void LoadMapReplaceList(const char* szFileName) override
	{
		const char* pszFileContent = (const char*)gEngfuncs.COM_LoadFile(szFileName, 5, NULL);

		if (!pszFileContent)
		{
			gEngfuncs.Con_DPrintf("LoadMapReplaceList: Could not load replace list file \"%s\"\n", szFileName);
			return;
		}

		LoadReplaceList(pszFileContent, m_MapEntries);

		gEngfuncs.COM_FreeFile((void*)pszFileContent);
	}

	/*
		Purpose:

			Fill the ReplacedFileName if szSourceFileName matched, by iterating m_Entries.
	*/

	bool ReplaceFileName(const char* szSourceFileName, std::string& ReplacedFileName) override
	{
		for (auto entry : m_MapEntries)
		{
			if (entry->GetReplacedFileName(szSourceFileName, ReplacedFileName))
			{
				return true;
			}
		}

		for (auto entry : m_GlobalEntries)
		{
			if (entry->GetReplacedFileName(szSourceFileName, ReplacedFileName))
			{
				return true;
			}
		}

		return false;
	}
};

static CResourceReplacer s_ModelReplacer;
static CResourceReplacer s_SoundReplacer;

IResourceReplacer* ModelReplacer()
{
	return &s_ModelReplacer;
}

IResourceReplacer* SoundReplacer()
{
	return &s_SoundReplacer;
}