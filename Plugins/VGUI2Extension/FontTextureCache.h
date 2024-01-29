#include "FontManager.h"
#include "tier1/utlrbtree.h"
#include <vgui/ISurface.h>

class CFontTextureCache
{
public:
	CFontTextureCache(void);
	~CFontTextureCache(void);

public:
	bool GetTextureForChar(HFont font, wchar_t wch, int *textureID, float **texCoords);
	bool AllocatePageForChar(int charWide, int charTall, int &pageIndex, int &drawX, int &drawY, int &twide, int &ttall);
	int ComputePageType(int charTall) const;

public:
	enum
	{
		FONT_PAGE_SIZE_16,
		FONT_PAGE_SIZE_32,
		FONT_PAGE_SIZE_64,
		FONT_PAGE_SIZE_128,

		FONT_PAGE_SIZE_COUNT,
	};

	typedef unsigned short HCacheEntry;

	struct CacheEntry_t
	{
		HFont font;
		wchar_t wch;
		unsigned char page;
		float texCoords[4];

		HCacheEntry nextEntry;
		HCacheEntry prevEntry;
	};

	struct Page_t
	{
		short textureID;
		short fontHeight;
		short wide, tall;
		short nextX, nextY;
	};

public:
	static bool CacheEntryLessFunc(CacheEntry_t const &lhs, CacheEntry_t const &rhs);

public:
	typedef CUtlVector<Page_t> FontPageList_t;
	CUtlRBTree<CacheEntry_t, HCacheEntry> m_CharCache;
	FontPageList_t m_PageList;
	int m_pCurrPage[FONT_PAGE_SIZE_COUNT];
	HCacheEntry m_LRUListHeadIndex;
	static int s_pFontPageSize[FONT_PAGE_SIZE_COUNT];
};

extern CFontTextureCache g_FontTextureCache;