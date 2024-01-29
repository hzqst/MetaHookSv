#pragma once

#include <sequence.h>
#include <vector>

const int maxHUDMessages = 16;

struct message_parms_t
{
	client_textmessage_t *pMessage;
	float time;
	int x, y;
	int totalWidth, totalHeight;
	int width;
	int lines;
	int lineLength;
	int length;
	int r, g, b;
	int text;
	int fadeBlend;
	float charTime;
	float fadeTime;
};

const int MESSAGE_ARG_LEN = 64;
const int MAX_MESSAGE_ARGS = 4;

typedef struct client_message_s
{
	client_textmessage_t *pMessage;
	unsigned int font;
	wchar_t args[MAX_MESSAGE_ARGS][MESSAGE_ARG_LEN];
	int numArgs;
	int hintMessage;
}client_message_t;

class CHudMessage
{
public:
	int VidInit(void);
	int Draw(void);
	void Init(void);
	void Reset(void);

public:
	int MsgFunc_HudText(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_HudTextArgs(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_SendAudio(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_SayText(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_TextMsg(const char* pszName, int iSize, void* pbuf);
public:	
	char *LookupString(const char *msg, int *msg_dest);

	void EnsureTextFitsInOneLineAndWrapIfHaveTo(int line);
	int GetHudFontHeight(void);
	void GetStringSize(const wchar_t *string, int *width, int *height);
	int DrawVGUI2String(wchar_t *msg, int x, int y, float r, float g, float b);
	float FadeBlend(float fadein, float fadeout, float hold, float localTime);
	int XPosition(float x, int width, int lineWidth);
	int YPosition(float y, int height);

	void MessageAdd(client_textmessage_t *newMessage, bool bIsDynamicMessage);
	int MessageAdd(client_textmessage_t *newMessage, float time, int hintMessage, int useSlot, unsigned int m_hFont, bool bIsDynamicMessage);
	void MessageScanNextChar(unsigned int font);
	void MessageScanStart(void);
	void MessageDrawScan(client_message_t *pClientMessage, float time, unsigned int font);

	void RetireDynamicMessage(client_textmessage_t *pMsg);
	int SayTextPrint(const char *pszBuf, int iBufSize, int clientIndex, char *sstr1, char *sstr2, char *sstr3, char *sstr4);

private:
	client_message_t m_pMessages[maxHUDMessages];
	float m_startTime[maxHUDMessages];
	message_parms_t m_parms;
	float m_gameTitleTime;
	client_textmessage_t *m_pGameTitle;
	vgui::HFont	m_hFont;

	std::vector<client_textmessage_t *> m_DynamicTextMessages;
};

class CHudMenu
{
public:
	int VidInit(void);
	int Draw(void);
	void Init(void);
	void Reset(void);

	bool SelectMenuItem(int menu_item);

	int DrawHudString(int xpos, int ypos, int iMaxX, char *szIt, int r, int g, int b);
	int DrawHudStringReverse(int xpos, int ypos, int iMinX, char *szString, int r, int g, int b);
	char *LocaliseTextString(const char *msg, char *dst_buffer, int buffer_size);
	char *BufferedLocaliseTextString(const char *msg);
public:
	int MsgFunc_ShowMenu(const char* pszName, int iSize, void* pbuf);
public:

#define MAX_MENU_STRING 512

	int m_bitsValidSlots;
	bool m_bMenuDisplayed;
	bool m_bIsASMenu;
	float m_flShutoffTime;
	int m_fWaitingForMore;
	char m_szMenuString[MAX_MENU_STRING];
	char m_szPrelocalisedMenuString[MAX_MENU_STRING];


	int menu_r;
	int menu_g;
	int menu_b;
	int menu_x;
	int menu_ralign;

	vgui::HFont m_hFont;
	int m_iFontEngineHeight;
};

struct message_parms_svclient_t
{
	client_textmessage_t *pMessage;
	float time;
	int x;
	int y;
	int totalWidth;
	int totalHeight;
	int width;
	int lines;
	int lineLength;
	int length;
	int r;
	int g;
	int b;
	int a;
	int svtext;
	int fadeBlend;
	float charTime;
	float fadeTime;
};

class CHudMessage_SvClient
{
	DWORD *vftable;
	char padding[144];
	message_parms_svclient_t m_parms;
};