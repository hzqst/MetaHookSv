#ifndef WIN32FONT_H
#define WIN32FONT_H

#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#include <windows.h>

#include "tier1/utlrbtree.h"

#ifdef GetCharABCWidths
#undef GetCharABCWidths
#endif

class CWin32Font;

class CWin32Font
{
public:
	struct abc_t
	{
		char a;
		char b;
		char c;
	};

	struct abc_cache_t
	{
		abc_t abc;
		wchar_t wch;
	};

	enum { ABCWIDTHS_CACHE_SIZE = 256 };

public:
	CWin32Font(void);
	~CWin32Font(void);

public:
	virtual bool Create(const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags);
	virtual void GetCharRGBA(int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba);
	virtual bool IsEqualTo(const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags);
	virtual bool IsValid(void);
	virtual void GetCharABCWidths(int ch, int &a, int &b, int &c);
	virtual void SetAsActiveFont(HDC hdc);
	virtual int GetHeight(void);
	virtual int GetMaxCharWidth(void);
	virtual int GetFlags(void);
	virtual const char *GetName(void) { return m_szName; }
	virtual bool GetUnderlined(void);
	virtual int GetAscent(void);
	virtual bool GetOutlined(void);

private:
	void ApplyScanlineEffectToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba);
	void ApplyGaussianBlurToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba);
	void ApplyDropShadowToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, int charWide, int charTall, unsigned char *rgba);
	void ApplyOutlineToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, int charWide, int charTall, unsigned char *rgba);
	void ApplyRotaryEffectToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba);

private:
	static bool ExtendedABCWidthsCacheLessFunc(const abc_cache_t &lhs, const abc_cache_t &rhs);
	static inline void GetBlurValueForPixel(unsigned char *src, int blur, float *gaussianDistribution, int x, int y, int wide, int tall, unsigned char *dest);

public:
	char m_szName[32];
	int m_iTall;
	int m_iWeight;
	int m_iFlags;
	bool m_bAntiAliased;
	bool m_bRotary;
	bool m_bAdditive;
	int m_iDropShadowOffset;
	bool m_bUnderlined;
	bool m_bOutlined;
	int m_iOutlineSize;
	int m_iHeight;
	int m_iMaxCharWidth;
	int m_iAscent;
	CUtlRBTree<abc_cache_t> m_ExtendedABCWidthsCache;
	int m_rgiBitmapSize[2];
	unsigned char *m_pBuf;
	int m_iScanLines;
	int m_iBlur;
	float *m_pGaussianDistribution;
	int m_iFxpHeight;
	HFONT m_hFont;
	HDC m_hDC;
	HBITMAP m_hDIB;
	abc_t m_ABCWidthsCache[ABCWIDTHS_CACHE_SIZE];

public:
	static bool ms_bSetFriendlyNameCacheLessFunc;
};

void __fastcall CWin32Font_GetCharRGBA(void *pthis, int, int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba);
#endif