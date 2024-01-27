#include <malloc.h>
#include "FontTextureCache.h"
#include <IEngineSurface.h>
#include "vgui_internal.h"

extern IEngineSurface *staticSurface;
extern IEngineSurface_HL25 *staticSurface_HL25;

CFontTextureCache g_FontTextureCache;

int CFontTextureCache::s_pFontPageSize[FONT_PAGE_SIZE_COUNT] =
{
	16,
	32,
	64,
	128,
};

CFontTextureCache::CFontTextureCache(void): m_CharCache(0, 256, CacheEntryLessFunc)
{
	CacheEntry_t listHead = { 0, 0 };
	m_LRUListHeadIndex = m_CharCache.Insert(listHead);

	m_CharCache[m_LRUListHeadIndex].nextEntry = m_LRUListHeadIndex;
	m_CharCache[m_LRUListHeadIndex].prevEntry = m_LRUListHeadIndex;

	for (int i = 0; i < FONT_PAGE_SIZE_COUNT; ++i)
		m_pCurrPage[i] = -1;
}

CFontTextureCache::~CFontTextureCache(void)
{
}

bool CFontTextureCache::GetTextureForChar(HFont font, wchar_t wch, int *textureID, float **texCoords)
{
	static CacheEntry_t cacheitem;

	cacheitem.font = font;
	cacheitem.wch = wch;

	HCacheEntry cacheHandle = m_CharCache.Find(cacheitem);

	if (m_CharCache.IsValidIndex(cacheHandle))
	{
		int page = m_CharCache[cacheHandle].page;
		*textureID = m_PageList[page].textureID;
		*texCoords = m_CharCache[cacheHandle].texCoords;
		return true;
	}

	CWin32Font *winFont = FontManager().GetFontForChar(font, wch);

	if (!winFont)
		return false;

	int fontTall = winFont->GetHeight();
	int a, b, c;
	winFont->GetCharABCWidths(wch, a, b, c);

	int fontWide = b;
	int page, drawX, drawY, twide, ttall;

	if (!AllocatePageForChar(fontWide, fontTall, page, drawX, drawY, twide, ttall))
		return false;

	int nByteCount = s_pFontPageSize[FONT_PAGE_SIZE_COUNT - 1] * s_pFontPageSize[FONT_PAGE_SIZE_COUNT - 1] * 4;
	unsigned char* rgba = (unsigned char *)malloc(nByteCount);
	if (rgba)
	{
		memset(rgba, 0, nByteCount);
		winFont->GetCharRGBA(wch, 0, 0, fontWide, fontTall, rgba);

		if (staticSurface)
		{
			staticSurface->drawSetTexture(m_PageList[page].textureID);
			staticSurface->drawSetSubTextureRGBA(m_PageList[page].textureID, drawX, drawY, rgba, fontWide, fontTall);
		}
		else
		{
			staticSurface_HL25->drawSetTexture(m_PageList[page].textureID);
			staticSurface_HL25->drawSetSubTextureRGBA(m_PageList[page].textureID, drawX, drawY, rgba, fontWide, fontTall);
		}
		free(rgba);
	}
	cacheitem.page = page;

	double adjust = 0.0f;
	cacheitem.texCoords[0] = (float)((double)drawX / ((double)twide + adjust));
	cacheitem.texCoords[1] = (float)((double)drawY / ((double)ttall + adjust));
	cacheitem.texCoords[2] = (float)((double)(drawX + fontWide) / (double)twide);
	cacheitem.texCoords[3] = (float)((double)(drawY + fontTall) / (double)ttall);
	m_CharCache.Insert(cacheitem);

	*textureID = m_PageList[page].textureID;
	*texCoords = cacheitem.texCoords;
	return true;
}

bool CFontTextureCache::AllocatePageForChar(int charWide, int charTall, int &pageIndex, int &drawX, int &drawY, int &twide, int &ttall)
{
	int nPageType = ComputePageType(charTall);
	pageIndex = m_pCurrPage[nPageType];

	int nNextX = 0;
	bool bNeedsNewPage = true;

	if (pageIndex > -1)
	{
		Page_t &page = m_PageList[pageIndex];
		nNextX = page.nextX + charWide;

		if (nNextX > page.wide)
		{
			page.nextX = 0;
			nNextX = charWide;
			page.nextY += page.fontHeight + 1;
		}

		bNeedsNewPage = ((page.nextY + page.fontHeight + 1) > page.tall);
	}

	if (bNeedsNewPage)
	{
		pageIndex = m_PageList.AddToTail();
		Page_t &newPage = m_PageList[pageIndex];
		m_pCurrPage[nPageType] = pageIndex;

		if (staticSurface)
			newPage.textureID = staticSurface->createNewTextureID();
		else
			newPage.textureID = staticSurface_HL25->createNewTextureID();

		newPage.fontHeight = s_pFontPageSize[nPageType];
		newPage.wide = 256;
		newPage.tall = 256;
		newPage.nextX = 0;
		newPage.nextY = 0;

		nNextX = charWide;

		unsigned char rgba[256 * 256 * 4];
		memset(rgba, 0, sizeof(rgba));

		if (staticSurface)
			staticSurface->drawSetTextureRGBA(newPage.textureID, rgba, newPage.wide, newPage.tall, false, true);
		else
			staticSurface_HL25->drawSetTextureRGBA(newPage.textureID, rgba, newPage.wide, newPage.tall, false, true);
	}

	Page_t &page = m_PageList[pageIndex];
	drawX = page.nextX;
	drawY = page.nextY;
	twide = page.wide;
	ttall = page.tall;

	page.nextX = nNextX + 1;
	return true;
}

int CFontTextureCache::ComputePageType(int charTall) const
{
	for (int i = 0; i < FONT_PAGE_SIZE_COUNT; ++i)
	{
		if (charTall < s_pFontPageSize[i])
			return i;
	}

	return -1;
}

bool CFontTextureCache::CacheEntryLessFunc(CacheEntry_t const &lhs, CacheEntry_t const &rhs)
{
	if (lhs.font < rhs.font)
		return true;
	else if (lhs.font > rhs.font)
		return false;

	return (lhs.wch < rhs.wch);
}