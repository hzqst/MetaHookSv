#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui.h>
#include <VGUI_controls/Controls.h>
#include "plugins.h"
#include "privatefuncs.h"
#include "exportfuncs.h"

HMODULE g_hVGui1 = NULL;

static void* g_vgui_TextImage_paint_orig = NULL;
static void** g_vftable_TextImage = NULL;
static void** g_vftable_Color = NULL;
static hook_t* g_phook_vgui_TextImage_paint = NULL;

class vgui1_IColor
{
public:
	virtual void init() = 0;
	virtual void setColor(int r, int g, int b, int a) = 0;
	virtual void setColor(int sc) = 0;
	virtual void getColor(int& r, int& g, int& b, int& a) = 0;
	virtual void getColor(int* sc) = 0;
};

class vgui1_Color
{
public:
	void** vftable;
	uchar color[4];
	int  schemeColor;
public:
	vgui1_Color()
	{
		vftable = NULL;
		color[0] = 0;
		color[1] = 0;
		color[2] = 0;
		color[3] = 0;
		schemeColor = 0;
	}
};

class vgui1_IFont
{
public:
	virtual void init(const char* name, void* pFileData, int fileDataLen, int tall, int wide, float rotation, int weight, bool italic, bool underline, bool strikeout, bool symbol) = 0;
	virtual void getCharRGBA(int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, uchar* rgba) = 0;
	virtual void getCharABCwide(int ch, int& a, int& b, int& c) = 0;
	virtual void getTextSize(const char* text, int& wide, int& tall) = 0;
	virtual int  getTall() = 0;
	virtual int  getId() = 0;
};

class vgui1_ITextImage
{
public:
	virtual void setPos(int x, int y) = 0;
	virtual void getPos(int& x, int& y) = 0;//+4
	virtual void getSize(int& wide, int& tall) = 0;//+8
	virtual void setColor(vgui1_Color& color) = 0;
	virtual void getColor(vgui1_Color* color) = 0;//+16
	virtual void setSize(int wide, int tall) = 0;
	virtual void drawSetColor(int sc) = 0;
	virtual void drawSetColor(int r, int g, int b, int a) = 0;
	virtual void drawFilledRect(int x0, int y0, int x1, int y1) = 0;
	virtual void drawOutlinedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void drawSetTextFont(int sf) = 0;
	virtual void drawSetTextFont(vgui1_IFont* font) = 0;
	virtual void drawSetTextColor(int* sc) = 0;
	virtual void drawSetTextColor(int r, int g, int b, int a) = 0;
	virtual void drawSetTextPos(int x, int y) = 0;
	virtual void drawPrintText(const char* str, int strlen) = 0;
	virtual void drawPrintText(int x, int y, const char* str, int strlen) = 0;
	virtual void drawPrintChar(char ch) = 0;
	virtual void drawPrintChar(int x, int y, char ch) = 0;
	virtual void drawSetTextureRGBA(int id, const char* rgba, int wide, int tall) = 0;
	virtual void drawSetTexture(int id) = 0;
	virtual void drawTexturedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void paint(void* panel) = 0;
	virtual void doPaint(void* panel) = 0;
	virtual void  init(int textBufferLen, const char* text) = 0;
	virtual void  getTextSize(int& wide, int& tall) = 0;
	virtual void  getTextSizeWrapped(int& wide, int& tall) = 0;
	virtual vgui1_IFont* getFont() = 0;//+108
	virtual void  setText(int textBufferLen, const char* text) = 0;
	virtual void  setText(const char* text) = 0;
	virtual void  setFont(int schemeFont) = 0;
	virtual void  setFont(vgui1_IFont* font) = 0;
};

class vgui1_TextImage
{
public:
	void** vftable;
	char padding[0x20];
	char* text;

public:
	vgui1_TextImage()
	{
		vftable = NULL;
		memset(padding, 0, sizeof(padding));
		text = NULL;
	}
};

void __fastcall vgui_TextImage_paint(vgui1_TextImage* pthis, int, void* panel)
{
	vgui1_ITextImage* pthis2 = (vgui1_ITextImage*)pthis;

	int wide, tall;
	pthis2->getSize(wide, tall);
	if (pthis->text)
	{
		qboolean isNonANSI = false;

		auto p = pthis->text;
		while (*p)
		{
			if ((*p) < 0 || (*p) > 127) {
				isNonANSI = true;
				break;
			}
			p++;
		}

		if (isNonANSI)
		{
			vgui1_Color color;
			color.vftable = g_vftable_Color;
			pthis2->getColor(&color);

			int r, g, b, a;
			vgui1_IColor* pColor = (vgui1_IColor*)&color;
			pColor->getColor(r, g, b, a);

			pthis2->drawSetTextColor(r, g, b, a);

			auto font = pthis2->getFont();
			int fontTall = font->getTall();

			vgui::HFont hFont = NULL;

			const char* fontName = "SmallScoreboardFont";

			if (fontTall >= 32)
				fontName = "LargeScoreboardFont";
			else if (fontTall >= 24)
				fontName = "MediumScoreboardFont";

			vgui::IScheme* pScheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetScheme("CaptionScheme"));
			if (pScheme)
			{
				hFont = pScheme->GetFont(fontName, true);
			}
			else
			{
				pScheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetDefaultScheme());
				if (pScheme)
				{
					hFont = pScheme->GetFont(fontName, true);
				}
			}
			if (hFont)
			{
				vgui::surface()->DrawSetTexture(0);
				vgui::surface()->DrawSetTextColor(r, g, b, 255);
				vgui::surface()->DrawSetTextPos(0, 0);
				vgui::surface()->DrawSetTextFont(hFont);

				wchar_t utext[256] = { 0 };
				vgui::localize()->ConvertANSIToUnicode(pthis->text, utext, sizeof(utext) - sizeof(wchar_t));

				vgui::surface()->DrawPrintText(utext, wcslen(utext));
				vgui::surface()->DrawFlushText();
				vgui::surface()->DrawSetTexture(0);
				return;
			}
		}
	}
	return gPrivateFuncs.vgui_TextImage_paint(pthis, 0, panel);
}

void VGUI1_InstallHooks(void)
{
	g_hVGui1 = GetModuleHandleA("vgui.dll");

	if (g_hVGui1)
	{
		g_vftable_TextImage = (void**)GetProcAddress(g_hVGui1, "??_7TextImage@vgui@@6B@");
		g_vftable_Color = (void**)GetProcAddress(g_hVGui1, "??_7Color@vgui@@6B@");

		gPrivateFuncs.vgui_TextImage_paint = (decltype(gPrivateFuncs.vgui_TextImage_paint))g_vftable_TextImage[22];

		Install_InlineHook(vgui_TextImage_paint);
	}
}

void VGUI1_Shutdown(void)
{
	Uninstall_Hook(vgui_TextImage_paint);
	g_hVGui1 = NULL;
}
