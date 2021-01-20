#include <interface.h>
#include "ICommandLine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const int MAX_PARAMETER_LEN = 128;

class CCommandLine : public ICommandLine
{
public:
	CCommandLine(void);
	virtual ~CCommandLine(void);

public:
	virtual void CreateCmdLine(const char *commandline);
	virtual void CreateCmdLine(int argc, char **argv);
	virtual const char *GetCmdLine(void) const;
	virtual	const char *CheckParm(const char *psz, const char **ppszValue = 0) const;

	virtual void RemoveParm(const char *parm);
	virtual void AppendParm(const char *pszParm, const char *pszValues);

	virtual int ParmCount(void);
	virtual int FindParm(const char *psz) const;
	virtual const char *GetParm(int nIndex);

	virtual const char *ParmValue(const char *psz, const char *pDefaultVal = NULL);
	virtual int ParmValue(const char *psz, int nDefaultVal);
	virtual float ParmValue(const char *psz, float flDefaultVal);

	virtual void SetParm(const char *pszParm, const char *pszValues);
	virtual void SetParm(const char *pszParm, int iValue);

private:
	enum
	{
		MAX_PARAMETER_LEN = 128,
		MAX_PARAMETERS = 256,
	};

	void LoadParametersFromFile(const char *&pSrc, char *&pDst, size_t maxDestLen, bool bInQuotes);
	void ParseCommandLine(void);
	void CleanUpParms(void);
	void AddArgument(const char *pFirst, const char *pLast);

private:
	char *m_pszCmdLine;
	int m_nParmCount;
	char *m_ppParms[MAX_PARAMETERS];
};

static CCommandLine g_CmdLine;

ICommandLine *CommandLine(void)
{
	return &g_CmdLine;
}

CCommandLine::CCommandLine(void)
{
	m_pszCmdLine = NULL;
	m_nParmCount = 0;
}

CCommandLine::~CCommandLine(void)
{
	CleanUpParms();
	delete [] m_pszCmdLine;
}

void CCommandLine::LoadParametersFromFile(const char *&pSrc, char *&pDst, size_t maxDestLen, bool bInQuotes)
{
	char szFileName[_MAX_PATH];
	char *pOut;
	char *pDestStart = pDst;

	if (maxDestLen < 3)
		return;

	pSrc++;
	pOut = szFileName;

	char terminatingChar = ' ';

	if (bInQuotes)
		terminatingChar = '\"';

	while (*pSrc && *pSrc != terminatingChar)
	{
		*pOut++ = *pSrc++;

		if ((pOut - szFileName) >= (_MAX_PATH - 1))
			break;
	}

	*pOut = '\0';

	if (*pSrc)
		pSrc++;

	FILE *fp = fopen(szFileName, "r");

	if (fp)
	{
		char c = (char)fgetc(fp);

		while (c != EOF)
		{
			if (c == '\n')
				c = ' ';

			*pDst++ = c;

			if ((size_t)(pDst - pDestStart) >= (maxDestLen - 2))
				break;

			c = (char)fgetc(fp);
		}

		*pDst++ = ' ';

		fclose(fp);
	}
	else
	{
		printf("Parameter file '%s' not found, skipping...", szFileName);
	}
}

void CCommandLine::CreateCmdLine(int argc, char **argv)
{
	char cmdline[2048];
	cmdline[0] = 0;

	const int MAX_CHARS = sizeof(cmdline) - 1;
	cmdline[MAX_CHARS] = 0;

	for (int i = 0; i < argc; ++i)
	{
		strncat(cmdline, "\"", MAX_CHARS);
		strncat(cmdline, argv[i], MAX_CHARS);
		strncat(cmdline, "\"", MAX_CHARS);
		strncat(cmdline, " ", MAX_CHARS);
	}

	CreateCmdLine(cmdline);
}

void CCommandLine::CreateCmdLine(const char *commandline)
{
	if (m_pszCmdLine)
		delete[] m_pszCmdLine;

	char szFull[4096];

	char *pDst = szFull;
	const char *pSrc = commandline;

	bool bInQuotes = false;
	const char *pInQuotesStart = 0;

	while (*pSrc)
	{
		if (*pSrc == '"')
		{
			if (pSrc == commandline || (pSrc[-1] != '/' && pSrc[-1] != '\\'))
			{
				bInQuotes = !bInQuotes;
				pInQuotesStart = pSrc + 1;
			}
		}

		if (*pSrc == '@')
		{
			if (pSrc == commandline || (!bInQuotes && isspace(pSrc[-1])) || (bInQuotes && pSrc == pInQuotesStart))
			{
				LoadParametersFromFile(pSrc, pDst, sizeof(szFull) - (pDst - szFull), bInQuotes);
				continue;
			}
		}

		if ((pDst - szFull) >= (sizeof(szFull) - 1))
			break;

		*pDst++ = *pSrc++;
	}

	*pDst = '\0';

	size_t len = strlen(szFull) + 1;
	m_pszCmdLine = new char [len];
	memcpy(m_pszCmdLine, szFull, len);

	ParseCommandLine();
}

static char *_stristr(char *pStr, const char *pSearch)
{
	if (!pStr || !pSearch)
		return 0;

	char *pLetter = pStr;

	while (*pLetter != 0)
	{
		if (tolower((unsigned char)*pLetter) == tolower((unsigned char)*pSearch))
		{
			char const *pMatch = pLetter + 1;
			char const *pTest = pSearch + 1;

			while (*pTest != 0)
			{
				if (*pMatch == 0)
					return 0;

				if (tolower((unsigned char)*pMatch) != tolower((unsigned char)*pTest))
					break;

				++pMatch;
				++pTest;
			}

			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
	}

	return 0;
}

void CCommandLine::RemoveParm(const char *pszParm)
{
	if (!m_pszCmdLine)
		return;

	char *p, *found;
	char *pnextparam;
	size_t n;
	size_t curlen;

	p = m_pszCmdLine;

	while (*p)
	{
		curlen = strlen(p);
		found = _stristr(p, pszParm);

		if (!found)
			break;

		pnextparam = found + 1;

		while (pnextparam && *pnextparam && (*pnextparam != ' '))
			pnextparam++;

		if (pnextparam && (static_cast<size_t>(pnextparam - found) > strlen(pszParm)))
		{
			p = pnextparam;
			continue;
		}

		while (pnextparam && *pnextparam && (*pnextparam != '-') && (*pnextparam != '+'))
			pnextparam++;

		if (pnextparam && *pnextparam)
		{
			n = curlen - (pnextparam - p);
			memcpy(found, pnextparam, n);
			found[n] = '\0';
		}
		else
		{
			n = pnextparam - found;
			memset(found, 0, n);
		}
	}

	while (1)
	{
		size_t len = strlen(m_pszCmdLine);

		if (len == 0 || m_pszCmdLine[len - 1] != ' ')
			break;

		m_pszCmdLine[len - 1] = '\0';
	}

	ParseCommandLine();
}

void CCommandLine::AppendParm(const char *pszParm, const char *pszValues)
{
	size_t nNewLength = 0;
	char *pCmdString;

	nNewLength = strlen(pszParm);

	if (pszValues)
		nNewLength += strlen(pszValues) + 1;

	nNewLength++;

	if (!m_pszCmdLine)
	{
		m_pszCmdLine = new char[nNewLength];
		strcpy(m_pszCmdLine, pszParm);

		if (pszValues)
		{
			strcat(m_pszCmdLine, " ");
			strcat(m_pszCmdLine, pszValues);
		}

		ParseCommandLine();
		return;
	}

	RemoveParm(pszParm);

	nNewLength += strlen(m_pszCmdLine) + 1 + 1;

	pCmdString = new char[nNewLength];
	memset(pCmdString, 0, nNewLength);

	strcpy(pCmdString, m_pszCmdLine);
	strcat(pCmdString, " ");
	strcat(pCmdString, pszParm);

	if (pszValues)
	{
		strcat(pCmdString, " ");
		strcat(pCmdString, pszValues);
	}

	delete [] m_pszCmdLine;
	m_pszCmdLine = pCmdString;

	ParseCommandLine();
}

const char *CCommandLine::GetCmdLine(void) const
{
	return m_pszCmdLine;
}

const char *CCommandLine::CheckParm(const char *psz, const char **ppszValue) const
{
	if (ppszValue)
		*ppszValue = NULL;

	int i = FindParm(psz);

	if (i == 0)
		return NULL;

	if (ppszValue)
	{
		if ((i + 1) >= m_nParmCount)
		{
			*ppszValue = NULL;
		}
		else
		{
			*ppszValue = m_ppParms[i+1];
		}
	}

	return m_ppParms[i];
}

void CCommandLine::AddArgument(const char *pFirst, const char *pLast)
{
	if (pLast == pFirst)
		return;

	if (m_nParmCount >= MAX_PARAMETERS)
		printf("CCommandLine::AddArgument: exceeded %d parameters", MAX_PARAMETERS);

	int nLen = (int)pLast - (int)pFirst + 1;
	m_ppParms[m_nParmCount] = new char [nLen];
	memcpy(m_ppParms[m_nParmCount], pFirst, nLen - 1);
	m_ppParms[m_nParmCount][nLen - 1] = 0;

	++m_nParmCount;
}

void CCommandLine::ParseCommandLine(void)
{
	CleanUpParms();

	if (!m_pszCmdLine)
		return;

	const char *pChar = m_pszCmdLine;

	while (*pChar && isspace(*pChar))
		++pChar;

	bool bInQuotes = false;
	const char *pFirstLetter = NULL;

	for ( ; *pChar; ++pChar)
	{
		if (bInQuotes)
		{
			if (*pChar != '\"')
				continue;

			AddArgument(pFirstLetter, pChar);
			pFirstLetter = NULL;
			bInQuotes = false;
			continue;
		}

		if (!pFirstLetter)
		{
			if (*pChar == '\"')
			{
				bInQuotes = true;
				pFirstLetter = pChar + 1;
				continue;
			}

			if (isspace(*pChar))
				continue;

			pFirstLetter = pChar;
			continue;
		}

		if (isspace(*pChar))
		{
			AddArgument(pFirstLetter, pChar);
			pFirstLetter = NULL;
		}
	}

	if (pFirstLetter)
	{
		AddArgument(pFirstLetter, pChar);
	}
}

void CCommandLine::CleanUpParms(void)
{
	for (int i = 0; i < m_nParmCount; ++i)
	{
		delete [] m_ppParms[i];
		m_ppParms[i] = NULL;
	}

	m_nParmCount = 0;
}

int CCommandLine::ParmCount(void)
{
	return m_nParmCount;
}

int CCommandLine::FindParm(const char *psz) const
{
	for (int i = 1; i < m_nParmCount; ++i)
	{
		if (!_stricmp(psz, m_ppParms[i]))
			return i;
	}

	return 0;
}

const char *CCommandLine::GetParm(int nIndex)
{
	if ((nIndex < 0) || (nIndex >= m_nParmCount))
		return "";

	return m_ppParms[nIndex];
}

const char *CCommandLine::ParmValue(const char *psz, const char *pDefaultVal)
{
	int nIndex = FindParm(psz);

	if ((nIndex == 0) || (nIndex == m_nParmCount - 1))
		return pDefaultVal;

	if (m_ppParms[nIndex + 1][0] == '-' || m_ppParms[nIndex + 1][0] == '+')
		return pDefaultVal;

	return m_ppParms[nIndex + 1];
}

int CCommandLine::ParmValue(const char *psz, int nDefaultVal)
{
	int nIndex = FindParm(psz);

	if ((nIndex == 0) || (nIndex == m_nParmCount - 1))
		return nDefaultVal;

	if (m_ppParms[nIndex + 1][0] == '-' || m_ppParms[nIndex + 1][0] == '+')
		return nDefaultVal;

	return atoi(m_ppParms[nIndex + 1]);
}

float CCommandLine::ParmValue(const char *psz, float flDefaultVal)
{
	int nIndex = FindParm(psz);

	if ((nIndex == 0 ) || (nIndex == m_nParmCount - 1))
		return flDefaultVal;

	if (m_ppParms[nIndex + 1][0] == '-' || m_ppParms[nIndex + 1][0] == '+')
		return flDefaultVal;

	return (float)atof(m_ppParms[nIndex + 1]);
}

void CCommandLine::SetParm(const char *pszParm, const char *pszValues)
{
	RemoveParm(pszParm);
	AppendParm(pszParm, pszValues);
}

void CCommandLine::SetParm(const char *pszParm, int iValue)
{
	char pszValue[64];
	_snprintf(pszValue, sizeof(pszValue), "%d", iValue);
	SetParm(pszParm, iValue);
}