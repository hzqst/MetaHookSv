#pragma once

#include <sequence.h>

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

typedef struct
{
	client_textmessage_t *pMessage;
	unsigned int font;
	wchar_t args[MAX_MESSAGE_ARGS][MESSAGE_ARG_LEN];
	int numArgs;
	int hintMessage;
}
client_message_t;

class CHudMessage
{
public:
	int VidInit(void);
	int Draw(void);
	void Init(void);
	void Reset(void);

public:
	float FadeBlend(float fadein, float fadeout, float hold, float localTime);
	int XPosition(float x, int width, int lineWidth);
	int YPosition(float y, int height);

public:
	int MsgFunc_HudText(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_HudTextArgs(const char *pszName, int iSize, void *pbuf);

public:
	void MessageAdd(client_textmessage_t *newMessage);
	int MessageAdd(client_textmessage_t *newMessage, float time, int hintMessage, unsigned int font);
	void MessageScanNextChar(unsigned int font);
	void MessageScanStart(void);
	void MessageDrawScan(client_message_t *pClientMessage, float time, unsigned int font);

private:
	client_message_t m_pMessages[maxHUDMessages];
	float m_startTime[maxHUDMessages];
	message_parms_t m_parms;
	float m_gameTitleTime;
	client_textmessage_t *m_pGameTitle;
	vgui::HFont	m_hFont;
};