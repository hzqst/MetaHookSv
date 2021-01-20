#include <vgui/VGUI.h>
#include "Win32Font.h"
#include "tier1/utlvector.h"

using vgui::HFont;

class CFontAmalgam
{
public:
	CFontAmalgam(void);
	~CFontAmalgam(void);

public:
	const char *Name(void);
	void SetName(const char *name);
	void AddFont(CWin32Font *font, int lowRange, int highRange);
	CWin32Font *GetFontForChar(int ch);
	int GetFontHeight(void);
	int GetFontMaxWidth(void);
	int GetFontLowRange(int i);
	int GetFontHighRange(int i);
	int GetFlags(int i);
	const char *GetFontName(int i);
	int GetCount(void);
	bool GetUnderlined(void);
	bool GetOutlined(void);

public:
	struct TFontRange
	{
		int lowRange;
		int highRange;
		CWin32Font *font;
	};

	CUtlVector<TFontRange> m_Fonts;
	char m_szName[32];
	int m_iMaxWidth;
	int m_iMaxHeight;
};