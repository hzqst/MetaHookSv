#include "FontAmalgam.h"
#include <tier0/dbg.h>
#include <vgui/VGUI.h>

CFontAmalgam::CFontAmalgam(void)
{
	m_Fonts.EnsureCapacity(4);
	m_iMaxHeight = 0;
	m_iMaxWidth = 0;
}

CFontAmalgam::~CFontAmalgam(void)
{
}

const char *CFontAmalgam::Name(void)
{
	return m_szName;
}

void CFontAmalgam::SetName(const char *name)
{
	strncpy(m_szName, name, sizeof(m_szName) - 1);
	m_szName[sizeof(m_szName) - 1] = 0;
}

void CFontAmalgam::AddFont(CWin32Font *font, int lowRange, int highRange)
{
	int i = m_Fonts.AddToTail();

	m_Fonts[i].font = font;
	m_Fonts[i].lowRange = lowRange;
	m_Fonts[i].highRange = highRange;

	m_iMaxHeight = max(font->GetHeight(), m_iMaxHeight);
	m_iMaxWidth = max(font->GetMaxCharWidth(), m_iMaxWidth);

	if (!m_iMaxWidth)
	{
		m_iMaxWidth = 0;
		return;
	}
}

CWin32Font *CFontAmalgam::GetFontForChar(int ch)
{
	for (int i = 0; i < m_Fonts.Count(); i++)
	{
		if (ch >= m_Fonts[i].lowRange && ch <= m_Fonts[i].highRange)
		{
			assert(m_Fonts[i].font->IsValid());
			return m_Fonts[i].font;
		}
	}

	return NULL;
}

int CFontAmalgam::GetFontHeight(void)
{
	if (!m_Fonts.Count())
		return m_iMaxHeight;

	return m_Fonts[0].font->GetHeight();
}

int CFontAmalgam::GetFontMaxWidth(void)
{
	return m_iMaxWidth;
}

int CFontAmalgam::GetFontLowRange(int i)
{
	return m_Fonts[i].lowRange;
}

int CFontAmalgam::GetFontHighRange(int i)
{
	return m_Fonts[i].highRange;
}

const char *CFontAmalgam::GetFontName(int i)
{
	if (m_Fonts[i].font)
		return m_Fonts[i].font->GetName();
	else
		return NULL;
}

int CFontAmalgam::GetFlags(int i)
{
	if (m_Fonts.Count() && m_Fonts[i].font)
		return m_Fonts[i].font->GetFlags();
	else
		return 0;
}

int CFontAmalgam::GetCount(void)
{
	return m_Fonts.Count();
}

bool CFontAmalgam::GetUnderlined(void)
{
	if (!m_Fonts.Count())
		return false;

	return m_Fonts[0].font->GetUnderlined();
}

bool CFontAmalgam::GetOutlined(void)
{
	if (!m_Fonts.Count())
		return false;

	return m_Fonts[0].font->GetOutlined();
}