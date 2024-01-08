#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "Win32Font.h"
#include <tier0/dbg.h>
#include <vgui/ISurface.h>
#include <tier0/mem.h>
#include <tier1/utlbuffer.h>
#include "FontManager.h"
#include <IFileSystem.h>
#include <tier2\tier2.h>

static OSVERSIONINFO s_OsVersionInfo;
static bool s_bOsVersionInitialized = false;
bool s_bSupportsUnicode = true;

bool CWin32Font::ms_bSetFriendlyNameCacheLessFunc;

bool CWin32Font::ExtendedABCWidthsCacheLessFunc(const abc_cache_t &lhs, const abc_cache_t &rhs)
{
	return lhs.wch < rhs.wch;
}

CWin32Font::CWin32Font(void): m_ExtendedABCWidthsCache(256, 0, &ExtendedABCWidthsCacheLessFunc)
{
	m_szName[0] = 0;
	m_iTall = 0;
	m_iWeight = 0;
	m_iHeight = 0;
	m_iAscent = 0;
	m_iFlags = 0;
	m_iMaxCharWidth = 0;
	m_hFont = NULL;
	m_hDC = NULL;
	m_hDIB = NULL;
	m_bAntiAliased = false;
	m_bUnderlined = false;
	m_iBlur = 0;
	m_iScanLines = 0;
	m_bRotary = false;
	m_bAdditive = false;
	m_rgiBitmapSize[0] = m_rgiBitmapSize[1] = 0;
	m_ExtendedABCWidthsCache.EnsureCapacity(128);
	m_pGaussianDistribution = NULL;

	memset(m_ABCWidthsCache, 0, sizeof(m_ABCWidthsCache));

	if (!s_bOsVersionInitialized)
	{
		s_bOsVersionInitialized = true;
		memset(&s_OsVersionInfo, 0, sizeof(s_OsVersionInfo));
		s_OsVersionInfo.dwOSVersionInfoSize = sizeof(s_OsVersionInfo);
		GetVersionEx(&s_OsVersionInfo);

		if (s_OsVersionInfo.dwMajorVersion >= 5)
			s_bSupportsUnicode = true;
		else
			s_bSupportsUnicode = false;
	}
}

CWin32Font::~CWin32Font(void)
{
	if (m_hFont)
		::DeleteObject(m_hFont);

	if (m_hDC)
		::DeleteDC(m_hDC);

	if (m_hDIB)
		::DeleteObject(m_hDIB);
}

bool g_bFontFound = false;

int CALLBACK FontEnumProc(const LOGFONT *lpelfe, const TEXTMETRIC *lpntme, DWORD FontType, LPARAM lParam)
{
	g_bFontFound = true;
	return 0;
}

bool CWin32Font::Create(const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags)
{
	strncpy(m_szName, windowsFontName, sizeof(m_szName) - 1);
	m_szName[sizeof(m_szName) - 1] = 0;

	m_iTall = tall;
	m_iWeight = weight;
	m_iFlags = flags;
	m_bAntiAliased = (flags & vgui::FONTFLAG_ANTIALIAS) ? 1 : 0;
	m_bUnderlined = flags & vgui::FONTFLAG_UNDERLINE;
	m_iDropShadowOffset = (flags & vgui::FONTFLAG_DROPSHADOW) ? 1 : 0;
	m_iOutlineSize = (flags & vgui::FONTFLAG_OUTLINE) ? 1 : 0;
	m_bOutlined = (flags & vgui::FONTFLAG_OUTLINE2) ? true : false;
	m_iBlur = blur;
	m_iScanLines = scanlines;
	m_bRotary = (flags & vgui::FONTFLAG_ROTARY) ? 1 : 0;
	m_bAdditive = (flags & vgui::FONTFLAG_ADDITIVE) ? 1 : 0;

	int iCharSet = (flags & vgui::FONTFLAG_SYMBOL) ? SYMBOL_CHARSET : DEFAULT_CHARSET;

	if (!stricmp(windowsFontName, "win98japanese"))
	{
		iCharSet = SHIFTJIS_CHARSET;
		strcpy(m_szName, "Tahoma");
	}

	m_hDC = ::CreateCompatibleDC(NULL);
	Assert(m_hDC);

	LOGFONT LogFont;
	LogFont.lfCharSet = DEFAULT_CHARSET;
	LogFont.lfPitchAndFamily = 0;
	strcpy(LogFont.lfFaceName, m_szName);

	g_bFontFound = false;
	::EnumFontFamiliesEx(m_hDC, &LogFont, &FontEnumProc, 0, 0);

	if (!g_bFontFound)
	{
		m_szName[0] = 0;
		return false;
	}

	m_hFont = ::CreateFontA(
		tall,
		0, 0, 0,
		m_iWeight, 
		flags & vgui::FONTFLAG_ITALIC, 
		flags & vgui::FONTFLAG_UNDERLINE,
		flags & vgui::FONTFLAG_STRIKEOUT, 
		iCharSet, 
		OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS,
		m_bAntiAliased ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, 
		windowsFontName);

	if (!m_hFont)
	{
		Error("Couldn't create windows font '%s'\n", windowsFontName);
		m_szName[0] = 0;
		return false;
	}

	::SetMapMode(m_hDC, MM_TEXT);
	::SelectObject(m_hDC, m_hFont);
	::SetTextAlign(m_hDC, TA_LEFT | TA_TOP | TA_UPDATECP);

	::TEXTMETRIC tm;
	memset(&tm, 0, sizeof(tm));

	if (!GetTextMetrics(m_hDC, &tm))
	{
		m_szName[0] = 0;
		return false;
	}

	m_iHeight = tm.tmHeight + m_iDropShadowOffset + 2 * m_iOutlineSize;
	m_iMaxCharWidth = tm.tmMaxCharWidth;
	m_iAscent = tm.tmAscent;

	m_rgiBitmapSize[0] = tm.tmMaxCharWidth + m_iOutlineSize * 2;
	m_rgiBitmapSize[1] = tm.tmHeight + m_iDropShadowOffset + m_iOutlineSize * 2;

	::BITMAPINFOHEADER header;
	memset(&header, 0, sizeof(header));
	header.biSize = sizeof(header);
	header.biWidth = m_rgiBitmapSize[0];
	header.biHeight = -m_rgiBitmapSize[1];
	header.biPlanes = 1;
	header.biBitCount = 32;
	header.biCompression = BI_RGB;

	m_hDIB = ::CreateDIBSection(m_hDC, (BITMAPINFO*)&header, DIB_RGB_COLORS, (void **)(&m_pBuf), NULL, 0);
	::SelectObject(m_hDC, m_hDIB);

	memset(m_ABCWidthsCache, 0, sizeof(m_ABCWidthsCache));
	ABC abc[ABCWIDTHS_CACHE_SIZE];
	assert(ABCWIDTHS_CACHE_SIZE <= 256);

	if (::GetCharABCWidthsW(m_hDC, 0, ABCWIDTHS_CACHE_SIZE - 1, &abc[0]) || ::GetCharABCWidthsA(m_hDC, 0, ABCWIDTHS_CACHE_SIZE - 1, &abc[0]))
	{	
		for (int i = 0; i < ABCWIDTHS_CACHE_SIZE; i++)
		{
			m_ABCWidthsCache[i].a = abc[i].abcA - m_iBlur - m_iOutlineSize;
			m_ABCWidthsCache[i].b = abc[i].abcB + ((m_iBlur + m_iOutlineSize) * 2) + m_iDropShadowOffset;
			m_ABCWidthsCache[i].c = abc[i].abcC - m_iBlur - m_iDropShadowOffset - m_iOutlineSize;
		}
	}
	else
	{
		for (int i = 0; i < ABCWIDTHS_CACHE_SIZE; i++)
			m_ABCWidthsCache[i].b = (char)tm.tmAveCharWidth;
	}

	if (m_iBlur > 1)
	{
		m_pGaussianDistribution = new float[m_iBlur * 2 + 1];
		double sigma = 0.683 * m_iBlur;

		for (int x = 0; x <= (m_iBlur * 2); x++)
		{
			int val = x - m_iBlur;
			m_pGaussianDistribution[x] = (float)(1.0f / sqrt(2 * 3.14 * sigma * sigma)) * pow(2.7, -1 * (val * val) / (2 * sigma * sigma));
			m_pGaussianDistribution[x] *= 1;
		}
	}

	return true;
}

void CWin32Font::GetCharRGBA(int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba)
{
	int a, b, c;
	GetCharABCWidths(ch, a, b, c);
	::SelectObject(m_hDC, m_hFont);

	int wide = b;

	if (m_bUnderlined)
		wide += (a + c);

	int tall = m_iHeight;
	GLYPHMETRICS glyphMetrics;
	MAT2 mat2 = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
	int bytesNeeded = 0;

	bool bShouldAntialias = m_bAntiAliased;

	if (ch > 0x00FF && !(m_iFlags & vgui::FONTFLAG_CUSTOM))
		bShouldAntialias = false;

	if (!s_bSupportsUnicode)
	{
		if (ch == 'I' || ch == '1')
			bShouldAntialias = false;

		if (m_iHeight >= 13)
			bShouldAntialias = false;
	}

	if (bShouldAntialias)
	{
		::SelectObject(m_hDC, m_hFont);
		bytesNeeded = ::GetGlyphOutlineW(m_hDC, ch, GGO_GRAY8_BITMAP, &glyphMetrics, 0, NULL, &mat2);
	}

	if (bytesNeeded > 0)
	{
		unsigned char *lpbuf = (unsigned char *)_alloca(bytesNeeded);
		::GetGlyphOutlineW(m_hDC, ch, GGO_GRAY8_BITMAP, &glyphMetrics, bytesNeeded, lpbuf, &mat2);

		wide = glyphMetrics.gmBlackBoxX;

		while (wide % 4 != 0)
			wide++;

		int pushDown = m_iAscent - glyphMetrics.gmptGlyphOrigin.y;
		int xstart = 0;

		if ((int)glyphMetrics.gmBlackBoxX >= b + 2)
			xstart = (glyphMetrics.gmBlackBoxX - b) / 2;

		for (unsigned int j = 0; j < glyphMetrics.gmBlackBoxY; j++)
		{
			for (unsigned int i = xstart; i < glyphMetrics.gmBlackBoxX; i++)
			{
				int x = i - xstart + m_iBlur + m_iOutlineSize;
				int y = j + pushDown;

				if ((x < rgbaWide) && (y < rgbaTall))
				{
					unsigned char grayscale = lpbuf[(j * wide + i)];
					float r, g, b, a;

					if (grayscale)
					{
						r = g = b = 1.0f;
						a = (grayscale + 0) / 64.0f;
						if (a > 1.0f) a = 1.0f;
					}
					else
						r = g = b = a = 0.0f;

					if (ch == '\t')
						r = g = b = 0;

					unsigned char *dst = &rgba[(y*rgbaWide+x)*4];
					dst[0] = (unsigned char)(r * 255.0f);
					dst[1] = (unsigned char)(g * 255.0f);
					dst[2] = (unsigned char)(b * 255.0f);
					dst[3] = (unsigned char)(a * 255.0f);
				}
			}
		}
	}
	else
	{
		::SetBkColor(m_hDC, RGB(0, 0, 0));
		::SetTextColor(m_hDC, RGB(255, 255, 255));
		::SetBkMode(m_hDC, OPAQUE);

		if (m_bUnderlined)
			::MoveToEx(m_hDC, 0, 0, NULL);
		else
			::MoveToEx(m_hDC, -a, 0, NULL);

		wchar_t wch = (wchar_t)ch;

		if (s_bSupportsUnicode)
		{
			::ExtTextOutW(m_hDC, 0, 0, 0, NULL, &wch, 1, NULL);
		}
		else
		{
			RECT rect = { 0, 0, wide, tall};
			::ExtTextOut(m_hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

			char mbcs[6] = { 0 };
			::WideCharToMultiByte(CP_ACP, 0, &wch, 1, mbcs, sizeof(mbcs), NULL, NULL);
			::ExtTextOutA(m_hDC, 0, 0, 0, NULL, mbcs, strlen(mbcs), NULL);
		}

		::SetBkMode(m_hDC, TRANSPARENT);

		if (wide > m_rgiBitmapSize[0])
			wide = m_rgiBitmapSize[0];

		if (tall > m_rgiBitmapSize[1])
			tall = m_rgiBitmapSize[1];

		for (int j = (int)m_iOutlineSize; j < tall - (int)m_iOutlineSize; j++)
		{
			for (int i = (int)m_iOutlineSize; i < wide - (int)m_iDropShadowOffset - (int)m_iOutlineSize; i++)
			{
				if ((i < rgbaWide) && (j < rgbaTall))
				{
					unsigned char *src = &m_pBuf[(i + j*m_rgiBitmapSize[0])*4];
					unsigned char *dst = &rgba[(i + j*rgbaWide)*4];
					unsigned char r, g, b;

					if (ch == '\t')
					{
						r = g = b = 0;
					}
					else
					{
						r = src[0];
						g = src[1];
						b = src[2];
					}

					dst[0] = r;
					dst[1] = g;
					dst[2] = b;
					dst[3] = (unsigned char)((float)r * 0.34f + (float)g * 0.55f + (float)b * 0.11f);
				}
			}
		}

		if (m_iDropShadowOffset)
		{
			unsigned char *dst = &rgba[((m_iHeight - 1) * rgbaWide) * 4];

			for (int i = 0; i < wide; i++)
			{
				dst[0] = 0;
				dst[1] = 0;
				dst[2] = 0;
				dst[3] = 0;
				dst += 4;
			}
		}
	}

	if (m_iDropShadowOffset)
		ApplyDropShadowToTexture(rgbaX, rgbaY, rgbaWide, rgbaTall, wide, tall, rgba);

	if (m_iOutlineSize)
		ApplyOutlineToTexture(rgbaX, rgbaY, rgbaWide, rgbaTall, wide, tall, rgba);

	ApplyGaussianBlurToTexture(rgbaX, rgbaY, rgbaWide, rgbaTall, rgba);
	ApplyScanlineEffectToTexture(rgbaX, rgbaY, rgbaWide, rgbaTall, rgba);
	ApplyRotaryEffectToTexture(rgbaX, rgbaY, rgbaWide, rgbaTall, rgba);
}

void CWin32Font::ApplyRotaryEffectToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba)
{
	if (!m_bRotary)
		return;

	int y = rgbaTall * 0.5;
	unsigned char *line = &rgba[(rgbaX + ((y + rgbaY) * rgbaWide)) * 4];

	for (int x = 0; x < rgbaWide; x++, line += 4)
	{
		line[0] = 127;
		line[1] = 127;
		line[2] = 127;
		line[3] = 255;
	}
}

void CWin32Font::ApplyScanlineEffectToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba)
{
	if (m_iScanLines < 2)
		return;

	for (int y = 0; y < rgbaTall; y++)
	{
		if (y % m_iScanLines == 0)
			continue;

		unsigned char *line = &rgba[(rgbaX + ((y + rgbaY) * rgbaWide)) * 4];

		for (int x = 0; x < rgbaWide; x++, line += 4)
		{
			line[0] *= 0.7;
			line[1] *= 0.7;
			line[2] *= 0.7;
		}
	}
}

void CWin32Font::ApplyDropShadowToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, int charWide, int charTall, unsigned char *rgba)
{
	for (int y = charTall - 1; y >= m_iDropShadowOffset; y--)
	{
		for (int x = charWide - 1; x >= m_iDropShadowOffset; x--)
		{
			unsigned char *dest = &rgba[(x + rgbaX + ((rgbaY + y) * rgbaWide)) * 4];

			if (dest[3] == 0)
			{
				unsigned char *src = &rgba[(x + rgbaX - m_iDropShadowOffset + ((rgbaY + y - m_iDropShadowOffset) * rgbaWide)) * 4];
				dest[0] = 0;
				dest[1] = 0;
				dest[2] = 0;
				dest[3] = src[3];
			}
		}
	}
}

void CWin32Font::ApplyOutlineToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, int charWide, int charTall, unsigned char *rgba)
{
	int x, y;

	for (y = 0; y < charTall; y++)
	{
		for (x = 0; x < charWide; x++)
		{
			unsigned char *src = &rgba[(x + rgbaX + ((rgbaY + y) * rgbaWide)) * 4];

			if (src[3] < 32)
			{
				int shadowX, shadowY;

				for (shadowX = -m_iOutlineSize; shadowX <= m_iOutlineSize; shadowX++)
				{
					for (shadowY = -m_iOutlineSize; shadowY <= m_iOutlineSize; shadowY++)
					{
						if (shadowX == 0 && shadowY == 0)
							continue;

						int testX = shadowX + x;
						int testY = shadowY + y;

						if (testX < 0 || testX >= charWide || testY < 0 || testY >= charTall)
							continue;

						unsigned char *test = &rgba[(testX + rgbaX + ((rgbaY + testY) * rgbaWide)) * 4];

						if (test[0] != 0 && test[1] != 0 && test[2] != 0 && test[3] != 0)
						{
							src[0] = 0;
							src[1] = 0;
							src[2] = 0;
							src[3] = 255;
						}
					}
				}
			}
		}
	}
}

void CWin32Font::ApplyGaussianBlurToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba)
{
	if (!m_pGaussianDistribution)
		return;

	unsigned char *src = (unsigned char *)_alloca(rgbaWide * rgbaTall * 4);
	memcpy(src, rgba, rgbaWide * rgbaTall * 4);
	unsigned char *dest = rgba;

	for (int y = 0; y < rgbaTall; y++)
	{
		for (int x = 0; x < rgbaWide; x++)
		{
			GetBlurValueForPixel(src, m_iBlur, m_pGaussianDistribution, x, y, rgbaWide, rgbaTall, dest);
			dest += 4;
		}
	}
}

void CWin32Font::GetBlurValueForPixel(unsigned char *src, int blur, float *gaussianDistribution, int srcX, int srcY, int srcWide, int srcTall, unsigned char *dest)
{
	int r = 0, g = 0, b = 0, a = 0;
	float accum = 0.0f;
	int maxX = min(srcX + blur, srcWide);
	int minX = max(srcX - blur, 0);

	for (int x = minX; x <= maxX; x++)
	{
		int maxY = min(srcY + blur, srcTall - 1);
		int minY = max(srcY - blur, 0);

		for (int y = minY; y <= maxY; y++)
		{
			unsigned char *srcPos = src + ((x + (y * srcWide)) * 4);
			float weight = gaussianDistribution[x - srcX + blur];
			float weight2 = gaussianDistribution[y - srcY + blur];
			accum += (srcPos[0] * (weight * weight2));
		}
	}

	r = g = b = a = (int)accum;

	int pixelCount = 1;
	dest[0] = min(r / pixelCount, 255);
	dest[1] = min(g / pixelCount, 255);
	dest[2] = min(b / pixelCount, 255);
	dest[3] = min(a / pixelCount, 255);
	dest[3] = 255;
}

bool CWin32Font::IsEqualTo(const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags)
{
	if (!stricmp(windowsFontName, m_szName) && m_iTall == tall && m_iWeight == weight && m_iBlur == blur && m_iFlags == flags)
		return true;

	return false;
}

bool CWin32Font::IsValid(void)
{
	if (m_szName[0])
		return true;

	return false;
}

void CWin32Font::SetAsActiveFont(HDC hdc)
{
	Assert(IsValid());
	::SelectObject(hdc, m_hFont);
}

void CWin32Font::GetCharABCWidths(int ch, int &a, int &b, int &c)
{
	Assert(IsValid());

	if (ch < ABCWIDTHS_CACHE_SIZE)
	{
		a = m_ABCWidthsCache[ch].a;
		b = m_ABCWidthsCache[ch].b;
		c = m_ABCWidthsCache[ch].c;
	}
	else
	{

		abc_cache_t finder;
		finder.wch = (wchar_t)ch;
		unsigned short i = m_ExtendedABCWidthsCache.Find(finder);

		if (m_ExtendedABCWidthsCache.IsValidIndex(i))
		{
			a = m_ExtendedABCWidthsCache[i].abc.a;
			b = m_ExtendedABCWidthsCache[i].abc.b;
			c = m_ExtendedABCWidthsCache[i].abc.c;
			return;
		}

		ABC abc;

		if (::GetCharABCWidthsW(m_hDC, ch, ch, &abc) || ::GetCharABCWidthsA(m_hDC, ch, ch, &abc))
		{
			a = abc.abcA;
			b = abc.abcB;
			c = abc.abcC;
		}
		else
		{
			SIZE size;
			char mbcs[6] = { 0 };
			wchar_t wch = ch;
			::WideCharToMultiByte(CP_ACP, 0, &wch, 1, mbcs, sizeof(mbcs), NULL, NULL);

			if (::GetTextExtentPoint32(m_hDC, mbcs, strlen(mbcs), &size))
			{
				a = c = 0;
				b = size.cx;
			}
			else
			{
				a = c = 0;
				b = m_iMaxCharWidth;
			}
		}

		finder.abc.a = a - m_iBlur - m_iOutlineSize;
		finder.abc.b = b + ((m_iBlur + m_iOutlineSize) * 2) + m_iDropShadowOffset;
		finder.abc.c = c - m_iBlur - m_iDropShadowOffset - m_iOutlineSize;
		m_ExtendedABCWidthsCache.Insert(finder);
	}
}

int CWin32Font::GetHeight(void)
{
	assert(IsValid());
	return m_iHeight;
}

int CWin32Font::GetMaxCharWidth(void)
{
	assert(IsValid());
	return m_iMaxCharWidth;
}

int CWin32Font::GetFlags(void)
{
	return m_iFlags;
}

bool CWin32Font::GetUnderlined(void)
{
	return m_bUnderlined;
}

bool CWin32Font::GetOutlined(void)
{
	return m_bOutlined;
}

int CWin32Font::GetBlur(void)
{
	return m_iBlur;
}

bool CWin32Font::GetAdditive(void)
{
	return m_bAdditive;
}

int CWin32Font::GetAscent(void)
{
	return m_iAscent;
}

void __fastcall CWin32Font_GetCharRGBA(void *pthis, int, int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba)
{
	CWin32Font *pFont = (CWin32Font *)pthis;
	pFont->GetCharRGBA(ch, rgbaX, rgbaY, rgbaWide, rgbaTall, rgba);
}