#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <VGUI_controls/Controls.h>
#include <VGUI_controls/Panel.h>
#include <VGUI_controls/Frame.h>
#include <IClientVGUI.h>
#include "privatefuncs.h"
#include "plugins.h"
#include "Viewport.h"

#include <vgui.h>

namespace vgui
{
bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories);
}

static void (__fastcall *m_pfnCClientVGUI_Initialize)(void *pthis, int,CreateInterfaceFn *factories, int count) = NULL;
static void (__fastcall *m_pfnCClientVGUI_Start)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_SetParent)(void *pthis, int, vgui::VPANEL parent) = NULL;
static bool (__fastcall *m_pfnCClientVGUI_UseVGUI1)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_HideScoreBoard)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_HideAllVGUIMenu)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_ActivateClientUI)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_HideClientUI)(void *pthis, int) = NULL;

class CClientVGUI : public IClientVGUI
{
public:
	virtual void Initialize(CreateInterfaceFn *factories, int count);
	virtual void Start(void);
	virtual void SetParent(vgui::VPANEL parent);
	virtual bool UseVGUI1(void);
	virtual void HideScoreBoard(void);
	virtual void HideAllVGUIMenu(void);
	virtual void ActivateClientUI(void);
	virtual void HideClientUI(void);
	virtual void unknown(void);
	virtual void Shutdown(void);
};

static CClientVGUI s_ClientVGUI;

IClientVGUI *g_pClientVGUI = NULL;

void CClientVGUI::Initialize(CreateInterfaceFn *factories, int count)
{
	m_pfnCClientVGUI_Initialize(this, 0, factories, count);

	vgui::VGui_InitInterfacesList("CaptionMod", factories, count);

	vgui::scheme()->LoadSchemeFromFile( "captionmod/CaptionScheme.res", "CaptionScheme" );

	if (!vgui::localize()->AddFile(g_pFullFileSystem, "captionmod/dictionary_%language%.txt"))
	{
		if (!vgui::localize()->AddFile(g_pFullFileSystem, "captionmod/dictionary_english.txt"))
		{
			g_pMetaHookAPI->SysError("Failed to load captionmod/dictionary_english.txt");
		}
	}
}

void CClientVGUI::Start(void)
{
	m_pfnCClientVGUI_Start(this, 0);

	g_pViewPort = new CViewport();
	g_pViewPort->Start();
}

void CClientVGUI::SetParent(vgui::VPANEL parent)
{
	m_pfnCClientVGUI_SetParent(this, 0, parent);

	g_pViewPort->SetParent(parent);
}

bool CClientVGUI::UseVGUI1(void)
{
	return m_pfnCClientVGUI_UseVGUI1(this, 0);
}

void CClientVGUI::HideScoreBoard(void)
{
	m_pfnCClientVGUI_HideScoreBoard(this, 0);
}

void CClientVGUI::HideAllVGUIMenu(void)
{
	m_pfnCClientVGUI_HideAllVGUIMenu(this, 0);
}

void CClientVGUI::ActivateClientUI(void)
{
	m_pfnCClientVGUI_ActivateClientUI(this, 0);

	g_pViewPort->ActivateClientUI();
}

void CClientVGUI::HideClientUI(void)
{
	m_pfnCClientVGUI_HideClientUI(this, 0);

	g_pViewPort->HideClientUI();
}

void CClientVGUI::unknown(void)
{

}

void CClientVGUI::Shutdown(void)
{

}

void ClientVGUI_InstallHook(void)
{
	CreateInterfaceFn ClientVGUICreateInterface = NULL;
	if(g_hClientDll)
		ClientVGUICreateInterface = (CreateInterfaceFn)Sys_GetFactory((HINTERFACEMODULE)g_hClientDll);
	if(!ClientVGUICreateInterface && gExportfuncs.ClientFactory)
		ClientVGUICreateInterface = (CreateInterfaceFn)gExportfuncs.ClientFactory();

	g_pClientVGUI = (IClientVGUI *)ClientVGUICreateInterface(CLIENTVGUI_INTERFACE_VERSION, NULL);

	if(g_pClientVGUI)
	{
		DWORD *pVFTable = *(DWORD **)&s_ClientVGUI;

		g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0,  1, (void *)pVFTable[1], (void **)&m_pfnCClientVGUI_Initialize);
		g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0,  2, (void *)pVFTable[2], (void **)&m_pfnCClientVGUI_Start);
		g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0,  3, (void *)pVFTable[3], (void **)&m_pfnCClientVGUI_SetParent);
		g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0,  7, (void *)pVFTable[7], (void **)&m_pfnCClientVGUI_ActivateClientUI);
		g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0,  8, (void *)pVFTable[8], (void **)&m_pfnCClientVGUI_HideClientUI);

		g_IsClientVGUI2 = true;
	}
}

//implement the ClientVGUI interface for those mod with no ClientVGUI implemented

class NewClientVGUI : public IClientVGUI
{
public:
	virtual void Initialize(CreateInterfaceFn *factories, int count);
	virtual void Start(void);
	virtual void SetParent(vgui::VPANEL parent);
	virtual bool UseVGUI1(void);
	virtual void HideScoreBoard(void);
	virtual void HideAllVGUIMenu(void);
	virtual void ActivateClientUI(void);
	virtual void HideClientUI(void);
	virtual void unknown(void);
	virtual void Shutdown(void);
};

void NewClientVGUI::Initialize(CreateInterfaceFn *factories, int count)
{
	vgui::VGui_InitInterfacesList("CaptionMod", factories, count);

	vgui::scheme()->LoadSchemeFromFile( "captionmod/CaptionScheme.res", "CaptionScheme" );

	if (!vgui::localize()->AddFile(g_pFullFileSystem, "captionmod/dictionary_%language%.txt"))
	{
		if (!vgui::localize()->AddFile(g_pFullFileSystem, "captionmod/dictionary_english.txt"))
		{
			g_pMetaHookAPI->SysError("Failed to load captionmod/dictionary_english.txt");
		}
	}
}

extern vgui::ISurface *g_pSurface;

void NewClientVGUI::Start(void)
{
	g_pViewPort = new CViewport();
	g_pViewPort->Start();

	//Fix a bug that VGUI1 mouse disappear
	auto pSurface4 = (DWORD)g_pSurface + 4;
	*(PUCHAR)(pSurface4 + 0x4B) = 0;
}

void NewClientVGUI::SetParent(vgui::VPANEL parent)
{
	g_pViewPort->SetParent(parent);
}

bool NewClientVGUI::UseVGUI1(void)
{
	return true;
}

void NewClientVGUI::HideScoreBoard(void)
{
}

void NewClientVGUI::HideAllVGUIMenu(void)
{
}

void NewClientVGUI::ActivateClientUI(void)
{
	g_pViewPort->ActivateClientUI();
}

void NewClientVGUI::HideClientUI(void)
{
	g_pViewPort->HideClientUI();
}

void NewClientVGUI::unknown(void)
{

}

void NewClientVGUI::Shutdown(void)
{
	
}

EXPOSE_SINGLE_INTERFACE(NewClientVGUI, IClientVGUI, CLIENTVGUI_INTERFACE_VERSION);

HMODULE g_hVGui1 = NULL;

void *vgui_TextImage_paint_orig = NULL;
void **vftable_TextImage = NULL;
void **vftable_Color = NULL;

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
	void **vftable;
	uchar color[4];
	int  schemeColor;
public:
	vgui1_Color()
	{
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
	virtual void init(const char* name, void *pFileData, int fileDataLen, int tall, int wide, float rotation, int weight, bool italic, bool underline, bool strikeout, bool symbol) = 0;
	virtual void getCharRGBA(int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, uchar* rgba) = 0;
	virtual void getCharABCwide(int ch, int& a, int& b, int& c) = 0;
	virtual void getTextSize(const char* text, int& wide, int& tall) = 0;
	virtual int  getTall() = 0;
	virtual int  getId()  = 0;
};

class vgui1_ITextImage
{
public:
	virtual void setPos(int x, int y) = 0;
	virtual void getPos(int& x, int& y) = 0;//+4
	virtual void getSize(int& wide, int& tall) = 0;//+8
	virtual void setColor(vgui1_Color &color) = 0;
	virtual void getColor(vgui1_Color* color) = 0;//+16
	virtual void setSize(int wide, int tall) = 0;
	virtual void drawSetColor(int sc) = 0;
	virtual void drawSetColor(int r, int g, int b, int a) = 0;
	virtual void drawFilledRect(int x0, int y0, int x1, int y1) = 0;
	virtual void drawOutlinedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void drawSetTextFont(int sf) = 0;
	virtual void drawSetTextFont(vgui1_IFont* font) = 0;
	virtual void drawSetTextColor(int *sc) = 0;
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
	void **vftable;
	char padding[0x20];
	char* text;
};

hook_t *g_phook_vgui_TextImage_paint = NULL;

void __fastcall vgui_TextImage_paint(vgui1_TextImage *pthis, int, void *panel)
{
	vgui1_ITextImage *pthis2 = (vgui1_ITextImage *)pthis;

	int wide, tall;
	pthis2->getSize(wide, tall);
	if (pthis->text)
	{
		qboolean isNonANSI = false;

		char *p = pthis->text;
		while (*p)
		{
			if (*p < 0 || *p > 127) {
				isNonANSI = true;
				break;
			}
			p++;
		}

		if (isNonANSI)
		{
			vgui1_Color color;
			memset(&color, 0, sizeof(color));
			color.vftable = vftable_Color;
			pthis2->getColor(&color);

			int r, g, b, a;
			vgui1_IColor *pColor = (vgui1_IColor *)&color;
			pColor->getColor(r, g, b, a);

			pthis2->drawSetTextColor(r, g, b, a);

			auto font = pthis2->getFont();
			int fontTall = font->getTall();
			
			vgui::HFont hFont = NULL;

			const char *fontName = "SmallScoreboardFont";

			if(fontTall >= 32)
				fontName = "LargeScoreboardFont";
			else if (fontTall >= 24)
				fontName = "MediumScoreboardFont";

			vgui::IScheme *pScheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetScheme("CaptionScheme"));
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

void VGUI1_InstallHook(void)
{
	g_hVGui1 = GetModuleHandleA("vgui.dll");

	if (g_hVGui1)
	{
		vftable_TextImage = (void **)GetProcAddress(g_hVGui1, "??_7TextImage@vgui@@6B@");
		vftable_Color = (void **)GetProcAddress(g_hVGui1, "??_7Color@vgui@@6B@");

		gPrivateFuncs.vgui_TextImage_paint = (decltype(gPrivateFuncs.vgui_TextImage_paint))vftable_TextImage[22];

		Install_InlineHook(vgui_TextImage_paint);
	}
}

void VGUI1_Shutdown(void)
{
	Uninstall_Hook(vgui_TextImage_paint);
}

void ClientVGUI_Shutdown(void)
{
	if (g_pViewPort)
	{
		delete g_pViewPort;
		g_pViewPort = NULL;
	}
}