#include <metahook.h>
#include "exportfuncs.h"
#include "parsemsg.h"
#include "privatefuncs.h"

#include <VGUI/VGUI.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include "Viewport.h"
#include "message.h"
#include <algorithm>

using namespace vgui;

extern cvar_t *hud_saytext;
extern cvar_t *hud_saytext_time;
extern cvar_t *cap_newchat;

char *m_pSenderName = NULL;

client_textmessage_t *g_pCurrentTextMessage = NULL;

CHudMessage m_HudMessage;
CHudMenu m_HudMenu;

pfnUserMsgHook m_pfnHudText;
pfnUserMsgHook m_pfnHudTextPro;
pfnUserMsgHook m_pfnHudTextArgs;
pfnUserMsgHook m_pfnSendAudio;
pfnUserMsgHook m_pfnSayText;
pfnUserMsgHook m_pfnTextMsg;
pfnUserMsgHook m_pfnShowMenu;

int __MsgFunc_TextMsg(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	if (m_HudMessage.MsgFunc_TextMsg(pszName, iSize, pbuf) != 0)
		return 1;

	return m_pfnTextMsg(pszName, iSize, pbuf);
}

int __MsgFunc_SayText(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	if (m_HudMessage.MsgFunc_SayText(pszName, iSize, pbuf) != 0)
		return 1;

	return m_pfnSayText(pszName, iSize, pbuf);
}

int __MsgFunc_HudText(const char *pszName, int iSize, void *pbuf)
{
	if(m_HudMessage.MsgFunc_HudText(pszName, iSize, pbuf) != 0)
		return 1;

	return m_pfnHudText(pszName, iSize, pbuf);
}

int __MsgFunc_HudTextPro(const char *pszName, int iSize, void *pbuf)
{
	if(m_HudMessage.MsgFunc_HudText(pszName, iSize, pbuf) != 0)
		return 1;

	return m_pfnHudTextPro(pszName, iSize, pbuf);
}

int __MsgFunc_HudTextArgs(const char *pszName, int iSize, void *pbuf)
{
	if(m_HudMessage.MsgFunc_HudTextArgs(pszName, iSize, pbuf) != 0)
		return 1;

	return m_pfnHudTextArgs(pszName, iSize, pbuf);
}

int __MsgFunc_SendAudio(const char* pszName, int iSize, void* pbuf)
{
	if (m_HudMessage.MsgFunc_SendAudio(pszName, iSize, pbuf) != 0)
		return 1;

	int result =  m_pfnSendAudio(pszName, iSize, pbuf);

	m_pSenderName = NULL;

	return result;
}

void CHudMessage::Init(void)
{
	m_pfnHudText = HOOK_MESSAGE(HudText);
	m_pfnHudTextPro = HOOK_MESSAGE(HudTextPro);
	m_pfnHudTextArgs = HOOK_MESSAGE(HudTextArgs);
	m_pfnSendAudio = HOOK_MESSAGE(SendAudio);
	m_pfnSayText = HOOK_MESSAGE(SayText);
	m_pfnTextMsg = HOOK_MESSAGE(TextMsg);
}

void CHudMessage::Reset(void)
{
	memset(m_pMessages, 0, sizeof(m_pMessages[0]) * maxHUDMessages);
	memset(m_startTime, 0, sizeof(m_startTime[0]) * maxHUDMessages);
}

int CHudMessage::VidInit(void)
{
	//Load EngineFont
	IScheme *pScheme = scheme()->GetIScheme(scheme()->GetScheme("CaptionScheme"));
	if(pScheme)
	{
		m_hFont = pScheme->GetFont("Legacy_CreditsFont", true);
		if(!m_hFont)
			m_hFont = pScheme->GetFont("CreditsFont", true);
	}
	else
	{
		pScheme = scheme()->GetIScheme(scheme()->GetDefaultScheme());
		if(pScheme)
		{
			m_hFont = pScheme->GetFont("Legacy_CreditsFont", true);
			if(!m_hFont)
				m_hFont = pScheme->GetFont("CreditsFont", true);
		}
	}

	Reset();

	return 1;
}

int CHudMessage::GetHudFontHeight(void)
{
	if (!m_hFont)
		return 0;

	return vgui::surface()->GetFontTall(m_hFont);
}

void CHudMessage::GetStringSize(const wchar_t *string, int *width, int *height)
{
	int i;
	int len;
	int a, b, c;

	if (width)
		*width = 0;

	if (height)
		*height = 0;

	if (!m_hFont)
		return;

	len = wcslen(string);

	if (width)
	{
		for (i = 0; i < len; i++)
		{
			vgui::surface()->GetCharABCwide(m_hFont, string[i], a, b, c);
			*width += a + b + c;
		}
	}

	if (height)
	{
		*height = GetHudFontHeight();
	}
}

int CHudMessage::DrawVGUI2String(wchar_t *msg, int x, int y, float r, float g, float b)
{
	int i;
	int iOriginalX;
	int iTotalLines;
	int iCurrentLine;
	int w1, w2, w3;
	bool bHorzCenter;
	int len;
	wchar_t *strTemp;
	int fontheight;

	if (!m_hFont)
		return 0;

	iCurrentLine = 0;
	iOriginalX = x;
	iTotalLines = 1;
	bHorzCenter = false;
	len = wcslen(msg);

	for (strTemp = msg; *strTemp; strTemp++)
	{
		if (*strTemp == '\r')
			iTotalLines++;
	}

	if (x == -1)
	{
		bHorzCenter = true;
	}

	if (y == -1)
	{
		fontheight = vgui::surface()->GetFontTall(m_hFont);
		y = (g_iVideoHeight - fontheight) / 2;
	}

	for (i = 0; i < iTotalLines; i++)
	{
		wchar_t line[1024];
		int iWidth;
		int iHeight;
		int iTempCount;
		int j;
		int shadow_x;

		iTempCount = 0;
		iWidth = 0;
		iHeight = 0;

		for (strTemp = &msg[iCurrentLine]; *strTemp; strTemp++, iCurrentLine++)
		{
			if (*strTemp == '\r')
				break;

			if (*strTemp != '\n')
				line[iTempCount++] = *strTemp;
		}

		line[iTempCount] = 0;

		GetStringSize(line, &iWidth, &iHeight);

		if (bHorzCenter)
			x = (g_iVideoWidth - iWidth) / 2;
		else
			x = iOriginalX;

		gEngfuncs.pfnDrawSetTextColor(0, 0, 0);

		shadow_x = x;

		for (j = 0; j < iTempCount; j++)
		{
			gEngfuncs.pfnVGUI2DrawCharacter(shadow_x, y, line[j], m_hFont);
			vgui::surface()->GetCharABCwide(m_hFont, line[j], w1, w2, w3);

			shadow_x += w1 + w2 + w3;
		}

		gEngfuncs.pfnDrawSetTextColor(r, g, b);

		for (j = 0; j < iTempCount; j++)
		{
			gEngfuncs.pfnVGUI2DrawCharacter(x, y, line[j], m_hFont);
			vgui::surface()->GetCharABCwide(m_hFont, line[j], w1, w2, w3);

			x += w1 + w2 + w3;
		}

		y += iHeight;
		iCurrentLine++;
	}

	return x;
}

float CHudMessage::FadeBlend(float fadein, float fadeout, float hold, float localTime)
{
	float fadeTime = fadein + hold;
	float fadeBlend;

	if (localTime < 0)
		return 0;

	if (localTime < fadein)
	{
		fadeBlend = 1 - ((fadein - localTime) / fadein);
	}
	else if (localTime > fadeTime)
	{
		if (fadeout > 0)
			fadeBlend = 1 - ((localTime - fadeTime) / fadeout);
		else
			fadeBlend = 0;
	}
	else
		fadeBlend = 1;

	return fadeBlend;
}

int CHudMessage::XPosition(float x, int width, int totalWidth)
{
	int xPos;

	if (x == -1)
	{
		xPos = (g_iVideoWidth - width) / 2;
	}
	else
	{
		if (x < 0)
			xPos = (1.0 + x) * g_iVideoWidth - totalWidth;
		else
			xPos = x * g_iVideoWidth;
	}

	if (xPos + width > g_iVideoWidth)
		xPos = g_iVideoWidth - width;
	else if (xPos < 0)
		xPos = 0;

	return xPos;
}

int CHudMessage::YPosition(float y, int height)
{
	int yPos;

	if (y == -1)
	{
		yPos = (g_iVideoHeight - height) * 0.5;
	}
	else
	{
		if (y < 0)
			yPos = (1.0 + y) * g_iVideoHeight - height;
		else
			yPos = y * g_iVideoHeight;
	}

	if (yPos + height > g_iVideoHeight)
		yPos = g_iVideoHeight - height;
	else if (yPos < 0)
		yPos = 0;

	return yPos;
}

void CHudMessage::MessageScanNextChar(unsigned int m_hFont)
{
	int srcRed, srcGreen, srcBlue, destRed, destGreen, destBlue;
	int blend;

	srcRed = m_parms.pMessage->r1;
	srcGreen = m_parms.pMessage->g1;
	srcBlue = m_parms.pMessage->b1;
	blend = 0;

	switch (m_parms.pMessage->effect)
	{
		case 0:
		case 1:
		{
			destRed = destGreen = destBlue = 0;
			blend = m_parms.fadeBlend;
			break;
		}

		case 2:
		{
			m_parms.charTime += m_parms.pMessage->fadein;

			if (m_parms.charTime > m_parms.time)
			{
				srcRed = srcGreen = srcBlue = 0;
				destRed = destGreen = destBlue = 0;
				blend = 0;
			}
			else
			{
				float deltaTime = m_parms.time - m_parms.charTime;

				destRed = destGreen = destBlue = 0;

				if (m_parms.time > m_parms.fadeTime)
				{
					blend = m_parms.fadeBlend;
				}
				else if (deltaTime > m_parms.pMessage->fxtime)
				{
					blend = 0;
				}
				else
				{
					destRed = m_parms.pMessage->r2;
					destGreen = m_parms.pMessage->g2;
					destBlue = m_parms.pMessage->b2;
					blend = 255 - (deltaTime * (1.0 / m_parms.pMessage->fxtime) * 255.0 + 0.5);
				}
			}

			break;
		}
	}

	if (blend > 255)
		blend = 255;
	else if (blend < 0)
		blend = 0;

	m_parms.r = ((srcRed * (255 - blend)) + (destRed * blend)) >> 8;
	m_parms.g = ((srcGreen * (255 - blend)) + (destGreen * blend)) >> 8;
	m_parms.b = ((srcBlue * (255 - blend)) + (destBlue * blend)) >> 8;

	if (m_parms.pMessage->effect == 1 && m_parms.charTime != 0)
	{
		if (m_parms.x >= 0 && m_parms.y >= 0)
			gEngfuncs.pfnVGUI2DrawCharacterAdd(m_parms.x, m_parms.y, m_parms.text, m_parms.pMessage->r2, m_parms.pMessage->g2, m_parms.pMessage->b2, m_hFont);
	}
}

void CHudMessage::MessageScanStart(void)
{
	switch (m_parms.pMessage->effect)
	{
		case 1:
		case 0:
		{
			m_parms.fadeTime = m_parms.pMessage->fadein + m_parms.pMessage->holdtime;

			if (m_parms.time < m_parms.pMessage->fadein)
			{
				m_parms.fadeBlend = ((m_parms.pMessage->fadein - m_parms.time) * (1.0 / m_parms.pMessage->fadein) * 255);
			}
			else if (m_parms.time > m_parms.fadeTime)
			{
				if (m_parms.pMessage->fadeout > 0)
					m_parms.fadeBlend = (((m_parms.time - m_parms.fadeTime) / m_parms.pMessage->fadeout) * 255);
				else
					m_parms.fadeBlend = 255;
			}
			else
				m_parms.fadeBlend = 0;

			m_parms.charTime = 0;

			if (m_parms.pMessage->effect == 1 && (rand() % 100) < 10)
				m_parms.charTime = 1;

			break;
		}

		case 2:
		{
			m_parms.fadeTime = (m_parms.pMessage->fadein * m_parms.length) + m_parms.pMessage->holdtime;

			if (m_parms.time > m_parms.fadeTime && m_parms.pMessage->fadeout > 0)
				m_parms.fadeBlend = (((m_parms.time - m_parms.fadeTime) / m_parms.pMessage->fadeout) * 255);
			else
				m_parms.fadeBlend = 0;

			break;
		}
	}
}

void CHudMessage::MessageDrawScan(client_message_t *pClientMessage, float time, unsigned int m_hFont)
{
	int i, j, length, width;
	wchar_t *pText;
	wchar_t line[256];
	wchar_t textBuf[2048];
	int a, b, c;
	char szTempMessage[256];
	client_textmessage_t *pMessage;
	int iMessageLength;
	wchar_t *pFullText;

	pMessage = pClientMessage->pMessage;

	if (!pMessage->pMessage || !*pMessage->pMessage)
		return;

	iMessageLength = strlen(pMessage->pMessage);

	m_parms.lines = 1;
	m_parms.time = time;
	m_parms.pMessage = pMessage;
	length = 0;
	width = 0;
	m_parms.totalWidth = 0;

	if (*pMessage->pMessage == '#')
	{
		strncpy(szTempMessage, pMessage->pMessage, sizeof(szTempMessage) - 1);
		szTempMessage[sizeof(szTempMessage) - 1] = 0;

		if (pMessage->pMessage[iMessageLength - 1] == '\r' || pMessage->pMessage[iMessageLength - 1] == '\n')
		{
			if (iMessageLength - 1 < sizeof(szTempMessage) - 1)
				szTempMessage[iMessageLength - 1] = 0;
		}

		pFullText = vgui::localize()->Find(szTempMessage);

		if (!pFullText)
		{
			gEngfuncs.Con_DPrintf("ERROR: Missing %s from the dictionary_english.txt file!\n", pMessage->pMessage);
			return;
		}

		if (pClientMessage->numArgs)
		{
			vgui::localize()->ConstructString(textBuf, sizeof(textBuf), pFullText, pClientMessage->numArgs, pClientMessage->args[0], pClientMessage->args[1], pClientMessage->args[2], pClientMessage->args[3]);
			pFullText = textBuf;
		}
	}
	else
	{
		vgui::localize()->ConvertANSIToUnicode(pMessage->pMessage, textBuf, sizeof(textBuf));
		pFullText = textBuf;
	}

	pText = pFullText;

	while (*pText)
	{
		if (*pText == '\n')
		{
			m_parms.lines++;

			if (width > m_parms.totalWidth)
				m_parms.totalWidth = width;

			width = 0;
		}
		else
		{
			vgui::surface()->GetCharABCwide(m_hFont, *pFullText, a, b, c);
			width += a + b + c;
		}

		pText++;
		length++;
	}

	m_parms.length = length;
	m_parms.totalHeight = vgui::surface()->GetFontTall(m_hFont) * m_parms.lines;

	m_parms.y = YPosition(pMessage->y, m_parms.totalHeight);
	pText = pFullText;

	m_parms.charTime = 0;

	MessageScanStart();

	for (i = 0; i < m_parms.lines; i++)
	{
		m_parms.lineLength = 0;
		m_parms.width = 0;

		while (*pText && *pText != '\n')
		{
			line[m_parms.lineLength] = *pText;

			vgui::surface()->GetCharABCwide(m_hFont, *pText, a, b, c);
			m_parms.width += a + b + c;
			m_parms.lineLength++;
			pText++;

			if (m_parms.lineLength >= sizeof(line) / sizeof(wchar_t) - 1)
				break;
		}

		pText++;
		line[m_parms.lineLength] = 0;

		m_parms.x = XPosition(pMessage->x, m_parms.width, m_parms.totalWidth);

		for (j = 0; j < m_parms.lineLength; j++)
		{
			m_parms.text = line[j];

			vgui::surface()->GetCharABCwide(m_hFont, m_parms.text, a, b, c);

			int next = m_parms.x + a + b + c;

			MessageScanNextChar(m_hFont);

			if (m_parms.x >= 0 && m_parms.y >= 0 && next <= g_iVideoWidth)
				gEngfuncs.pfnVGUI2DrawCharacterAdd(m_parms.x, m_parms.y, m_parms.text, m_parms.r, m_parms.g, m_parms.b, m_hFont);

			m_parms.x = next;
		}

		m_parms.y += vgui::surface()->GetFontTall(m_hFont);
	}
}

void CHudMessage::RetireDynamicMessage(client_textmessage_t *pMsg)
{
	for (auto itor = m_DynamicTextMessages.begin(); itor != m_DynamicTextMessages.end();)
	{
		if ((*itor) == pMsg)
		{
			itor = m_DynamicTextMessages.erase(itor);

			delete pMsg->pMessage;
			delete pMsg;

			return;
		}
		else
		{
			itor++;
		}
	}

}

int CHudMessage::Draw(void)
{
	int i;
	client_textmessage_t *pMessage;
	float endTime;

	float fTime = (*cl_time);

	for (i = 0; i < maxHUDMessages; i++)
	{
		if (m_pMessages[i].pMessage)
		{
			pMessage = m_pMessages[i].pMessage;

			if (m_startTime[i] > (*cl_time) || m_startTime[i] == 1.0)
				m_startTime[i] = (*cl_time) + m_parms.time - m_startTime[i] + 0.2;
		}
	}

	for (i = 0; i < maxHUDMessages; i++)
	{
		if (m_pMessages[i].pMessage)
		{
			pMessage = m_pMessages[i].pMessage;

			switch (pMessage->effect)
			{
				case 0:
				case 1:
				{
					endTime = m_startTime[i] + pMessage->fadein + pMessage->fadeout + pMessage->holdtime;
					break;
				}

				case 2:
				{
					endTime = m_startTime[i] + (pMessage->fadein * strlen(pMessage->pMessage)) + pMessage->fadeout + pMessage->holdtime;
					break;
				}
			}

			if (fTime <= endTime)
			{
				float messageTime = fTime - m_startTime[i];

				MessageDrawScan(&m_pMessages[i], messageTime, m_pMessages[i].font);
			}
			else
			{
				RetireDynamicMessage(m_pMessages[i].pMessage);

				m_pMessages[i].pMessage = NULL;
				m_pMessages[i].font = 0;
			}
		}
	}

	m_parms.time = (*cl_time);

	return 1;
}

int CHudMessage::MsgFunc_HudText(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	char *pString = READ_STRING();
	int hintMessage = READ_BYTE();

	if (!READ_OK())
		hintMessage = 0;

	CDictionary *dict = NULL;

	// Trim off a leading # if it's there

	if (!V_strncmp(pString, "__NETMESSAGE__", sizeof("__NETMESSAGE__") - 1))
	{
		if (cap_netmessage && cap_netmessage->value)
		{
			client_textmessage_t *pTextMessage = NULL;

			if (pString[0] == '#')
				pTextMessage = gPrivateFuncs.pfnTextMessageGet(pString + 1);
			else
				pTextMessage = gPrivateFuncs.pfnTextMessageGet(pString);

			if (pTextMessage)
			{
				int useSlot = -1;
				char *slotString = &pString[sizeof("__NETMESSAGE__") - 1];
				if (isdigit(*slotString))
				{
					useSlot = atoi(slotString);
				}

				std::smatch result;
				std::string str = pTextMessage->pMessage;

				dict = g_pViewPort->FindDictionary(str.c_str(), DICT_NETMESSAGE);

				if (!dict)
					dict = g_pViewPort->FindDictionaryRegex(str, DICT_NETMESSAGE, result);

				if (cap_debug && cap_debug->value)
				{
					gEngfuncs.Con_Printf((dict) ? "CaptionMod: NetMessage [%s] found.\n" : "CaptionMod: NetMessage [%s] not found.\n", pTextMessage->pMessage);
				}

				if (dict && dict->m_bRegex && result.size() > 1)
				{
					auto pMsg = new client_textmessage_t;
					memcpy(pMsg, pTextMessage, sizeof(*pTextMessage));
					pMsg->pMessage = (const char *)new char[HUDMESSAGE_MAXLENGTH];

					if (dict->m_bOverrideColor)
					{
						pMsg->r1 = dict->m_Color1.r();
						pMsg->g1 = dict->m_Color1.g();
						pMsg->b1 = dict->m_Color1.b();
						pMsg->a1 = dict->m_Color1.a();

						pMsg->r2 = dict->m_Color2.r();
						pMsg->g2 = dict->m_Color2.g();
						pMsg->b2 = dict->m_Color2.b();
						pMsg->a1 = dict->m_Color2.a();
					}

					if (dict->m_bOverrideDuration)
					{
						pMsg->holdtime = dict->m_flDuration;
					}

					std::string sentence;
					sentence.resize(HUDMESSAGE_MAXLENGTH);

					int finalLength = localize()->ConvertUnicodeToANSI(dict->m_szSentence.data(), (char *)sentence.data(), sentence.length());

					sentence.resize(finalLength);

					for (size_t i = 1; i < result.size(); ++i)
					{
						char temp[32] = { 0 };
						V_snprintf(temp, sizeof(temp), "{%d}", i);

						std::string src = temp;
						auto dst = result[i].str();

						int pos = -1;
						int curPos = 0;
						while (-1 != (pos = sentence.find(src, curPos)))
						{
							sentence.replace(pos, src.size(), dst);
							curPos = pos + dst.size();
						}
					}

					V_strncpy((char *)pMsg->pMessage, sentence.data(), HUDMESSAGE_MAXLENGTH - 1);
					((char *)pMsg->pMessage)[HUDMESSAGE_MAXLENGTH - 1] = 0;

					int slotNum = MessageAdd(pMsg, (*cl_time), hintMessage, useSlot, m_hFont, true);
					
					g_pCurrentTextMessage = pMsg;

					g_pViewPort->StartNextSubtitle(dict);

					g_pCurrentTextMessage = NULL;

					if (slotNum == -1)
					{
						delete pMsg->pMessage;
						delete pMsg;
					}

					m_parms.time = (*cl_time);
					return 1;
				}
				else if (dict && !dict->m_bRegex)
				{
					auto pMsg = new client_textmessage_t;
					memcpy(pMsg, pTextMessage, sizeof(*pTextMessage));
					pMsg->pMessage = (const char *)new char[HUDMESSAGE_MAXLENGTH];

					if (dict->m_bOverrideColor)
					{
						pMsg->r1 = dict->m_Color1.r();
						pMsg->g1 = dict->m_Color1.g();
						pMsg->b1 = dict->m_Color1.b();
						pMsg->a1 = dict->m_Color1.a();

						pMsg->r2 = dict->m_Color2.r();
						pMsg->g2 = dict->m_Color2.g();
						pMsg->b2 = dict->m_Color2.b();
						pMsg->a1 = dict->m_Color2.a();
					}

					if (dict->m_bOverrideDuration)
					{
						pMsg->holdtime = dict->m_flDuration;
					}

					char sentence[HUDMESSAGE_MAXLENGTH];

					int finalLength = localize()->ConvertUnicodeToANSI(dict->m_szSentence.data(), (char *)sentence, HUDMESSAGE_MAXLENGTH);

					sentence[finalLength] = 0;

					V_strncpy((char *)pMsg->pMessage, sentence, HUDMESSAGE_MAXLENGTH - 1);
					((char *)pMsg->pMessage)[HUDMESSAGE_MAXLENGTH - 1] = 0;

					int slotNum = MessageAdd(pMsg, (*cl_time), hintMessage, useSlot, m_hFont, true);

					g_pCurrentTextMessage = pMsg;

					g_pViewPort->StartNextSubtitle(dict);

					g_pCurrentTextMessage = NULL;

					if (slotNum == -1)
					{
						delete pMsg->pMessage;
						delete pMsg;
					}

					m_parms.time = (*cl_time);
					return 1;
				}
				else if (pTextMessage)
				{
					int slotNum = MessageAdd(pTextMessage, (*cl_time), hintMessage, useSlot, m_hFont, false);

					m_parms.time = (*cl_time);
					return 1;
				}
			}
		}

		return 0;
	}
	else
	{
		if (cap_hudmessage && cap_hudmessage->value)
		{
			dict = g_pViewPort->FindDictionary(pString, DICT_MESSAGE);

			if (cap_debug && cap_debug->value)
			{
				gEngfuncs.Con_Printf((dict) ? "CaptionMod: TextMessage [%s] found.\n" : "CaptionMod: TextMessage [%s] not found.\n", pString);
			}

			if (dict)
			{
				client_textmessage_t *pTextMessage = NULL;

				if (pString[0] == '#')
					pTextMessage = gPrivateFuncs.pfnTextMessageGet(pString + 1);
				else
					pTextMessage = gPrivateFuncs.pfnTextMessageGet(pString);

				if (pTextMessage)
				{
					auto pMsg = new client_textmessage_t;
					memcpy(pMsg, pTextMessage, sizeof(*pTextMessage));

					if (dict->m_bOverrideColor)
					{
						pMsg->r1 = dict->m_Color1.r();
						pMsg->g1 = dict->m_Color1.g();
						pMsg->b1 = dict->m_Color1.b();
						pMsg->a1 = dict->m_Color1.a();

						pMsg->r2 = dict->m_Color2.r();
						pMsg->g2 = dict->m_Color2.g();
						pMsg->b2 = dict->m_Color2.b();
						pMsg->a1 = dict->m_Color2.a();
					}

					if (dict->m_bOverrideDuration)
					{
						pMsg->holdtime = dict->m_flDuration;
					}

					pMsg->pMessage = (const char *)new char[HUDMESSAGE_MAXLENGTH];

					char sentence[HUDMESSAGE_MAXLENGTH];

					int finalLength = localize()->ConvertUnicodeToANSI(dict->m_szSentence.data(), (char *)sentence, HUDMESSAGE_MAXLENGTH);

					sentence[finalLength] = 0;

					V_strncpy((char *)pMsg->pMessage, sentence, HUDMESSAGE_MAXLENGTH - 1);
					((char *)pMsg->pMessage)[HUDMESSAGE_MAXLENGTH - 1] = 0;

					int slotNum = MessageAdd(pMsg, (*cl_time), hintMessage, -1, m_hFont, true);

					g_pCurrentTextMessage = pMsg;

					g_pViewPort->StartNextSubtitle(dict);

					g_pCurrentTextMessage = NULL;

					if (slotNum == -1)
					{
						delete pMsg->pMessage;
						delete pMsg;
					}

					m_parms.time = (*cl_time);
					return 1;
				}
			}
		}
	}
	
	return 0;
}

int CHudMessage::MsgFunc_HudTextArgs(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	char *pString = READ_STRING();
	int hintMessage = READ_BYTE();

	if (cap_hudmessage && cap_hudmessage->value)
	{
		CDictionary *dict = g_pViewPort->FindDictionary(pString, DICT_MESSAGE);

		if (cap_debug && cap_debug->value)
		{
			gEngfuncs.Con_Printf((dict) ? "CaptionMod: TextMessage [%s] found.\n" : "CaptionMod: TextMessage [%s] not found.\n", pString);
		}

		if (dict)
		{
			client_textmessage_t *pTextMessage = NULL;

			if (pString[0] == '#')
				pTextMessage = gPrivateFuncs.pfnTextMessageGet(pString + 1);
			else
				pTextMessage = gPrivateFuncs.pfnTextMessageGet(pString);

			if (pTextMessage)
			{
				auto pMsg = new client_textmessage_t;
				memcpy(pMsg, pTextMessage, sizeof(*pTextMessage));

				if (dict->m_bOverrideColor)
				{
					pMsg->r1 = dict->m_Color1.r();
					pMsg->g1 = dict->m_Color1.g();
					pMsg->b1 = dict->m_Color1.b();
					pMsg->a1 = dict->m_Color1.a();

					pMsg->r2 = dict->m_Color2.r();
					pMsg->g2 = dict->m_Color2.g();
					pMsg->b2 = dict->m_Color2.b();
					pMsg->a1 = dict->m_Color2.a();
				}

				if (dict->m_bOverrideDuration)
				{
					pMsg->holdtime = dict->m_flDuration;
				}

				pMsg->pMessage = (const char *)new char[HUDMESSAGE_MAXLENGTH];

				char sentence[HUDMESSAGE_MAXLENGTH];

				int finalLength = localize()->ConvertUnicodeToANSI(dict->m_szSentence.data(), (char *)sentence, HUDMESSAGE_MAXLENGTH);

				sentence[finalLength] = 0;

				V_strncpy((char *)pMsg->pMessage, sentence, HUDMESSAGE_MAXLENGTH - 1);
				((char *)pMsg->pMessage)[HUDMESSAGE_MAXLENGTH - 1] = 0;

 				int slotNum = MessageAdd(pMsg, (*cl_time), hintMessage, -1, m_hFont, true);

				if (slotNum != -1)
				{
					m_pMessages[slotNum].numArgs = max(min(0, READ_BYTE()), MAX_MESSAGE_ARGS);

					for (int i = 0; i < m_pMessages[slotNum].numArgs; i++)
					{
						char *tmp = READ_STRING();

						if (!tmp)
							tmp = "";

						vgui::localize()->ConvertANSIToUnicode(tmp, m_pMessages[slotNum].args[i], MESSAGE_ARG_LEN);
					}
				}

				g_pCurrentTextMessage = pMsg;

				g_pViewPort->StartNextSubtitle(dict);

				g_pCurrentTextMessage = NULL;

				if (slotNum == -1)
				{
					delete pMsg->pMessage;
					delete pMsg;
				}

				m_parms.time = (*cl_time);
				return 1;
			}
		}
	}

	return 0;
}


int CHudMessage::MsgFunc_SendAudio(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int entIndex = READ_BYTE();
	char* pString = READ_STRING();
	int pitch = READ_SHORT();

	if (!READ_OK())
		pitch = 0;

	hud_player_info_t info;
	gEngfuncs.pfnGetPlayerInfo(entIndex, &info);

	m_pSenderName = info.name;

	CDictionary* dict = g_pViewPort->FindDictionary(pString, DICT_SENDAUDIO);

	if (cap_debug && cap_debug->value)
	{
		gEngfuncs.Con_Printf((dict) ? "CaptionMod: SendAudio [%s] found.\n" : "CaptionMod: SendAudio [%s] not found.\n", pString);
	}

	if (dict)
	{
		g_pViewPort->StartSubtitle(dict);
	}

	return 0;
}

#define MAX_LINES 5
#define MAX_CHARS_PER_LINE 256

#define MAX_LINE_WIDTH (g_iVideoWidth - 40)
#define LINE_START 10

#define MAX_PLAYER_NAME_LENGTH 128
#define TEXTCOLOR_NORMAL 1
#define TEXTCOLOR_USEOLDCOLORS 2
#define TEXTCOLOR_PLAYERNAME 3
#define TEXTCOLOR_LOCATION 4
#define TEXTCOLOR_MAX 5

static float SCROLL_SPEED = 5;

struct TextRange
{
	int start;
	int end;
	float *color;
};

class SayTextLine
{
public:
	SayTextLine(void);

public:
	void Clear(void);
	void Draw(int x, int y, float r, float g, float b);
	void Colorize(void);
	void SetText(wchar_t *buf, int clientIndex);

public:
	wchar_t m_line[MAX_CHARS_PER_LINE];
	CUtlVector<TextRange> m_textRanges;
	int m_clientIndex;
	float *m_teamColor;

public:
	SayTextLine &operator = (SayTextLine &other);
};

SayTextLine::SayTextLine(void)
{
	Clear();
}

void SayTextLine::Clear(void)
{
	m_textRanges.RemoveAll();
	m_line[0] = 0;
}

void SayTextLine::Draw(int x, int y, float r, float g, float b)
{
	return;

	int rangeIndex;
	TextRange *range;
	wchar_t ch;

	if (m_textRanges.Size() != 0)
	{
		for (rangeIndex = 0; rangeIndex < m_textRanges.Size(); rangeIndex++)
		{
			range = &m_textRanges[rangeIndex];

			ch = m_line[range->end];
			m_line[range->end] = 0;

			if (range->color)
				x = m_HudMessage.DrawVGUI2String(m_line + range->start, x, y, range->color[0], range->color[1], range->color[2]);
			else
				x = m_HudMessage.DrawVGUI2String(m_line + range->start, x, y, r, g, b);

			m_line[range->end] = ch;
		}
	}
	else
	{
		m_HudMessage.DrawVGUI2String(m_line, x, y, r, g, b);
		return;
	}
}

SayTextLine &SayTextLine::operator = (SayTextLine &other)
{
	m_textRanges.RemoveAll();
	m_textRanges.SetCount(other.m_textRanges.Count());

	for (int i = 0; i < other.m_textRanges.Size(); i++)
		m_textRanges[i] = other.m_textRanges[i];

	m_clientIndex = other.m_clientIndex;
	m_teamColor[0] = other.m_teamColor[0];
	m_teamColor[1] = other.m_teamColor[1];
	m_teamColor[2] = other.m_teamColor[2];

	memcpy(m_line, other.m_line, sizeof(m_line));
	return *this;
}

void SayTextLine::Colorize(void)
{
	wchar_t *txt = m_line;
	int lineLen = wcslen(m_line);

	if (m_line[0] == TEXTCOLOR_PLAYERNAME || m_line[0] == TEXTCOLOR_LOCATION || m_line[0] == TEXTCOLOR_NORMAL)
	{
		while (txt && *txt)
		{
			TextRange range;
			int count;

			switch (*txt)
			{
			case TEXTCOLOR_PLAYERNAME:
			case TEXTCOLOR_LOCATION:
			case TEXTCOLOR_NORMAL:
			{
				range.start = (txt - m_line) + 1;

				switch (*txt)
				{
				case TEXTCOLOR_PLAYERNAME:
				{
					range.color = m_teamColor;
					break;
				}

				/*case TEXTCOLOR_LOCATION:
				{
					range.color = g_LocationColor;
					break;
				}*/

				default:
				{
					range.color = NULL;
				}
				}

				range.end = lineLen;
				count = m_textRanges.Count();

				if (count)
					m_textRanges[count - 1].end = range.start - 1;

				m_textRanges.AddToTail(range);
				break;
			}
			}

			txt++;
		}
	}
	else if (m_line[0] == TEXTCOLOR_USEOLDCOLORS)
	{
		wchar_t wName[128];
		hud_player_info_t playerinfo = { 0 };
		gEngfuncs.pfnGetPlayerInfo(m_clientIndex, &playerinfo);
		char *pName = playerinfo.name ? playerinfo.name : "";

		vgui::localize()->ConvertANSIToUnicode(pName, wName, sizeof(wName));

		if (pName)
		{
			wchar_t *nameInString = wcsstr(m_line, wName);

			if (nameInString)
			{
				TextRange range;
				range.start = 1;
				range.end = wcslen(wName) + (nameInString - m_line);
				//range.color = GetClientColor(m_clientIndex);
				m_textRanges.AddToTail(range);

				range.start = range.end;
				range.end = wcslen(m_line);
				range.color = NULL;
				m_textRanges.AddToTail(range);
			}
		}
	}

	if (!m_textRanges.Size())
	{
		TextRange range;
		range.start = 0;
		range.end = wcslen(m_line);
		range.color = NULL;
		m_textRanges.AddToTail(range);
	}
}

void SayTextLine::SetText(wchar_t *buf, int clientIndex)
{
	Clear();

	if (buf)
	{
		wcsncpy(m_line, buf, MAX_CHARS_PER_LINE);
		m_line[MAX_CHARS_PER_LINE - 1] = 0;
		m_clientIndex = clientIndex;
		m_teamColor = NULL;

		if (clientIndex > 0)
		{
			if (gPrivateFuncs.GetClientColor)
			{
				m_teamColor = gPrivateFuncs.GetClientColor(clientIndex);
			}

			Colorize();
		}
		else
		{
			TextRange range;
			range.start = 0;
			range.end = wcslen(m_line);
			range.color = NULL;
			m_textRanges.AddToTail(range);
		}
	}
}

static SayTextLine g_sayTextLine[MAX_LINES + 1];
static float flScrollTime = 0;

static int Y_START = 0;
static int line_height = 0;

int ScrollTextUp(void)
{
	g_sayTextLine[MAX_LINES].Clear();
	memmove(&g_sayTextLine[0], &g_sayTextLine[1], sizeof(g_sayTextLine) - sizeof(g_sayTextLine[0]));
	g_sayTextLine[MAX_LINES - 1].Clear();

	if (g_sayTextLine[0].m_line[0] == ' ')
	{
		g_sayTextLine[0].m_line[0] = 2;
		return 1 + ScrollTextUp();
	}

	return 1;
}

void CHudMessage::EnsureTextFitsInOneLineAndWrapIfHaveTo(int line)
{
	int line_width = 0;
	GetStringSize(g_sayTextLine[line].m_line, &line_width, &line_height);

	if ((line_width + LINE_START) > MAX_LINE_WIDTH)
	{
		int length = LINE_START;
		int tmp_len = 0;
		wchar_t *last_break = NULL;

		for (wchar_t *x = g_sayTextLine[line].m_line; *x != 0; x++)
		{
			if (x[0] == '/' && x[1] == '(')
			{
				x += 2;

				while (*x != 0 && *x != ')')
					x++;

				if (*x != 0)
					x++;

				if (*x == 0)
					break;
			}

			wchar_t buf[2];
			buf[1] = 0;

			if (*x == ' ' && x != g_sayTextLine[line].m_line)
				last_break = x;

			buf[0] = *x;
			GetStringSize(buf, &tmp_len, &line_height);
			length += tmp_len;

			if (length > MAX_LINE_WIDTH)
			{
				if (!last_break)
					last_break = x - 1;

				x = last_break;

				int j;

				do
				{
					for (j = 0; j < MAX_LINES; j++)
					{
						if (!*g_sayTextLine[j].m_line)
							break;
					}

					if (j == MAX_LINES)
					{
						int linesmoved = ScrollTextUp();
						line -= linesmoved;
						last_break = last_break - (sizeof(g_sayTextLine[0].m_line) * linesmoved);
					}
				} while (j == MAX_LINES);

				if ((wchar_t)*last_break == (wchar_t)' ')
				{
					int linelen = wcslen(g_sayTextLine[j].m_line);
					int remaininglen = wcslen(last_break);

					if ((linelen - remaininglen) <= MAX_CHARS_PER_LINE)
					{
						wcsncat(g_sayTextLine[j].m_line, last_break, MAX_CHARS_PER_LINE);
						g_sayTextLine[j].m_line[MAX_CHARS_PER_LINE - 1] = 0;
						g_sayTextLine[j].Colorize();
					}
				}
				else
				{
					if ((wcslen(g_sayTextLine[j].m_line) - wcslen(last_break) - 2) < MAX_CHARS_PER_LINE)
					{
						wcsncat(g_sayTextLine[j].m_line, L" ", MAX_CHARS_PER_LINE);
						wcsncat(g_sayTextLine[j].m_line, last_break, MAX_CHARS_PER_LINE);
						g_sayTextLine[j].m_line[MAX_CHARS_PER_LINE - 1] = 0;
						g_sayTextLine[j].Colorize();
					}
				}

				*last_break = 0;

				EnsureTextFitsInOneLineAndWrapIfHaveTo(j);
				break;
			}
		}
	}
}

int CHudMessage::SayTextPrint(const char *pszBuf, int iBufSize, int clientIndex, char *sstr1, char *sstr2, char *sstr3, char *sstr4)
{
	if (!g_pViewPort->AllowedToPrintText())
	{
		gEngfuncs.pfnConsolePrint(pszBuf);
		return 0;
	}

	int lineNum;
	wchar_t finalBuffer[MAX_CHARS_PER_LINE];
	const char *localized;
	char buf[MAX_CHARS_PER_LINE];
	int len;
	wchar_t *msg;
	wchar_t temp[MAX_CHARS_PER_LINE];
	wchar_t out[MAX_CHARS_PER_LINE];

	for (lineNum = 0; lineNum < MAX_LINES; lineNum++)
	{
		if (!g_sayTextLine[lineNum].m_line[0])
			break;
	}

	if (lineNum == MAX_LINES)
	{
		ScrollTextUp();
		lineNum = MAX_LINES - 1;
	}

	len = strlen(pszBuf);

	if (pszBuf[len - 1] == '\n' || pszBuf[len - 1] == '\r')
	{
		strncpy(buf, pszBuf, sizeof(buf) - 1);
		buf[len - 1] = 0;
		localized = buf;
	}
	else
	{
		strncpy(buf, pszBuf, sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = 0;
		localized = buf;
	}

	msg = vgui::localize()->Find(localized);

	if (!msg)
	{
		wchar_t *wsz;
		wchar_t locTerm[128];
		bool inword;
		int charsLeft;
		wchar_t *wordPtr;
		int outPos;
		wchar_t ch;
		int wordLen;
		wchar_t *localizedTerm;
		int num;

		vgui::localize()->ConvertANSIToUnicode(localized, temp, sizeof(temp));

		if (temp[0])
		{
			wsz = temp;
			charsLeft = wcslen(temp);
			inword = false;
			wordPtr = NULL;
			outPos = 0;

			while (charsLeft)
			{
				ch = *wsz;

				if (inword)
				{
					if (iswspace(ch) || charsLeft == 1)
					{
						wordLen = wsz - wordPtr;
						wcsncpy(locTerm, wordPtr, wordLen + 1);

						if (charsLeft == 1)
							locTerm[wordLen + 1] = 0;
						else
							locTerm[wordLen] = 0;

						if (vgui::localize()->ConvertUnicodeToANSI(locTerm, buf, sizeof(buf)))
							localizedTerm = vgui::localize()->Find(buf);
						else
							localizedTerm = locTerm;

						if (!localizedTerm)
							localizedTerm = locTerm;

						num = min((int)wcslen(localizedTerm), MAX_CHARS_PER_LINE - outPos);
						wcsncpy(&out[outPos], localizedTerm, num);
						outPos += num;

						if (charsLeft > 1)
						{
							out[outPos] = ch;
							outPos++;
						}

						inword = false;
					}
				}
				else
				{
					if (ch == '#')
					{
						inword = true;
						wordPtr = wsz;
					}
					else
					{
						if (outPos < MAX_CHARS_PER_LINE)
						{
							out[outPos] = ch;
							outPos++;
						}
					}
				}

				wsz++;
				charsLeft--;
			}

			if (outPos < MAX_CHARS_PER_LINE)
				out[outPos] = 0;
			else
				out[MAX_CHARS_PER_LINE - 1] = 0;
		}

		msg = out;
	}

	static wchar_t wstr[4][256];
	wchar_t *w[4];
	char *sptrs[4];
	int j;

	sptrs[0] = sstr1;
	sptrs[1] = sstr2;
	sptrs[2] = sstr3;
	sptrs[3] = sstr4;

	w[0] = NULL;
	w[1] = NULL;
	w[2] = NULL;
	w[3] = NULL;

	for (j = 0; j < 4; j++)
	{
		if (sptrs[j][0] == '#')
		{
			w[j] = vgui::localize()->Find(sptrs[j]);
		}

		if (!w[j])
		{
			vgui::localize()->ConvertANSIToUnicode(sptrs[j], wstr[j], sizeof(wstr[j]));
			w[j] = wstr[j];
		}
	}

	auto hasStdFormatString = wcsstr(msg, L"%s");
	auto useStdPrintf = false;
	
	if (hasStdFormatString && !(hasStdFormatString[2] > '0' && hasStdFormatString[2] < '9'))
		useStdPrintf = true;

	if (wcsstr(msg, L"%%"))
		useStdPrintf = true;

	if (useStdPrintf)
	{
		_snwprintf(finalBuffer, sizeof(finalBuffer), msg, w[0], w[1], w[2], w[3]);
	}
	else
	{
		vgui::localize()->ConstructString(finalBuffer, sizeof(finalBuffer), msg, 4, w[0], w[1], w[2], w[3]);
	}

	g_sayTextLine[lineNum].SetText(finalBuffer, clientIndex);

	EnsureTextFitsInOneLineAndWrapIfHaveTo(lineNum);

	if (lineNum == 0)
		flScrollTime = (*cl_time) + hud_saytext_time->value;

	//m_iFlags |= HUD_ACTIVE;

	//Y_START = GetTextPrintY();

	g_pViewPort->ChatPrintf(clientIndex, finalBuffer);

	char ufinalBuffer[256] = {0};
	Q_UnicodeToUTF8(finalBuffer, ufinalBuffer, sizeof(ufinalBuffer));
	V_strncat(ufinalBuffer, "\n", sizeof(ufinalBuffer), 1);

	gEngfuncs.pfnConsolePrint(ufinalBuffer);

	return 1;
}

int CHudMessage::MsgFunc_SayText(const char* pszName, int iSize, void* pbuf)
{
	if (!cap_newchat->value)
		return 0;

	BEGIN_READ(pbuf, iSize);

	int client_index = READ_BYTE();

	char formatStr[256];
	strncpy(formatStr, READ_STRING(), sizeof(formatStr) - 1);
	formatStr[255] = 0;

	char sstr1[256], sstr2[256], sstr3[256], sstr4[256];
	strncpy(sstr1, READ_STRING(), sizeof(sstr1) - 1);
	sstr1[255] = 0;

	strncpy(sstr2, READ_STRING(), sizeof(sstr2) - 1);
	sstr2[255] = 0;

	strncpy(sstr3, READ_STRING(), sizeof(sstr3) - 1);
	sstr3[255] = 0;

	strncpy(sstr4, READ_STRING(), sizeof(sstr4) - 1);
	sstr4[255] = 0;

	if (!sstr1[0] && client_index > 0)
	{
		hud_player_info_t playerinfo = { 0 };
		gEngfuncs.pfnGetPlayerInfo(client_index, &playerinfo);
		if (playerinfo.name)
		{
			strcpy(sstr1, playerinfo.name);
		}
	}

	return SayTextPrint(formatStr, iSize - 1, client_index, sstr1, sstr2, sstr3, sstr4);
}

#define HUD_PRINTNOTIFY 1
#define HUD_PRINTCONSOLE 2
#define HUD_PRINTTALK 3
#define HUD_PRINTCENTER 4
#define HUD_PRINTRADIO 5
#define MSG_BUF_SIZE 128

char *CHudMessage::LookupString(const char *msg, int *msg_dest)
{
	if (!msg)
		return "";

	if (msg[0] == '#')
	{
		client_textmessage_t *clmsg = gPrivateFuncs.pfnTextMessageGet(msg + 1);

		if (!clmsg || !(clmsg->pMessage))
		{
			return (char *)msg;
		}

		if (msg_dest)
		{
			if (clmsg->effect < 0)
				*msg_dest = -clmsg->effect;
		}

		return (char *)clmsg->pMessage;
	}
	else
	{
		return (char *)msg;
	}
}

void StripEndNewlineFromString(char *str)
{
	int s = strlen(str) - 1;

	if (s >= 0)
	{
		if (str[s] == '\n' || str[s] == '\r')
			str[s] = 0;
	}
}

int CHudMessage::MsgFunc_TextMsg(const char* pszName, int iSize, void* pbuf)
{
	if (!cap_newchat->value)
		return 0;

	char szBuf[6][MSG_BUF_SIZE];
	char szNewBuf[6][MSG_BUF_SIZE];

	BEGIN_READ(pbuf, iSize);

	int msg_dest = READ_BYTE();
	
	if (msg_dest == HUD_PRINTTALK || msg_dest == HUD_PRINTRADIO)
	{
		int playerIndex = -1;

		if (msg_dest == HUD_PRINTRADIO)
		{
			char *tmp = READ_STRING();

			if (tmp)
				playerIndex = atoi(tmp);
		}

		char *msg_text = LookupString(READ_STRING(), &msg_dest);
		msg_text = strncpy(szBuf[0], msg_text, MSG_BUF_SIZE - 1);
		szBuf[0][MSG_BUF_SIZE - 1] = 0;

		char *sstr1 = LookupString(READ_STRING(), NULL);
		sstr1 = strncpy(szBuf[1], sstr1, MSG_BUF_SIZE);
		szBuf[1][MSG_BUF_SIZE - 1] = 0;
		StripEndNewlineFromString(sstr1);

		char *sstr2 = LookupString(READ_STRING(), NULL);
		sstr2 = strncpy(szBuf[2], sstr2, MSG_BUF_SIZE);
		szBuf[2][MSG_BUF_SIZE - 1] = 0;
		StripEndNewlineFromString(sstr2);

		char *sstr3 = LookupString(READ_STRING(), NULL);
		sstr3 = strncpy(szBuf[3], sstr3, MSG_BUF_SIZE);
		szBuf[3][MSG_BUF_SIZE - 1] = 0;
		StripEndNewlineFromString(sstr3);

		char *sstr4 = LookupString(READ_STRING(), NULL);
		sstr4 = strncpy(szBuf[4], sstr4, MSG_BUF_SIZE);
		szBuf[4][MSG_BUF_SIZE - 1] = 0;
		StripEndNewlineFromString(sstr4);

		if (g_pViewPort && !g_pViewPort->AllowedToPrintText())
			return 0;

		switch (msg_dest)
		{
		case HUD_PRINTRADIO:
		{

			if (strlen(sstr1) > 0)
			{
				snprintf(szNewBuf[1], MSG_BUF_SIZE-1, "#%s", sstr1);
				szNewBuf[1][MSG_BUF_SIZE - 1] = 0;

				if (vgui::localize()->Find(szNewBuf[1]))
					sstr1 = szNewBuf[1];
			}

			if (strlen(sstr2) > 0)
			{
				snprintf(szNewBuf[2], MSG_BUF_SIZE, "#%s", sstr2);
				szNewBuf[2][MSG_BUF_SIZE - 1] = 0;

				if (vgui::localize()->Find(szNewBuf[2]))
					sstr2 = szNewBuf[2];
			}

			if (strlen(sstr3) > 0)
			{
				snprintf(szNewBuf[3], MSG_BUF_SIZE, "#%s", sstr3);
				szNewBuf[3][MSG_BUF_SIZE - 1] = 0;

				if (vgui::localize()->Find(szNewBuf[3]))
					sstr3 = szNewBuf[3];
			}

			if (strlen(sstr4) > 0)
			{
				snprintf(szNewBuf[4], MSG_BUF_SIZE, "#%s", sstr4);
				szNewBuf[4][MSG_BUF_SIZE - 1] = 0;

				if (vgui::localize()->Find(szNewBuf[4]))
					sstr4 = szNewBuf[4];
			}

			return SayTextPrint(msg_text, MSG_BUF_SIZE, playerIndex, sstr1, sstr2, sstr3, sstr4);
			break;
		}
		case HUD_PRINTTALK:
		{
#if 0//This is what vanilla HL client does and may cause potential invalid memory access if server sends bad format string
			snprintf(szBuf[5], MSG_BUF_SIZE, msg_text, sstr1, sstr2, sstr3, sstr4);
			szBuf[5][MSG_BUF_SIZE - 1] = 0;
			return SayTextPrint(szBuf[5], MSG_BUF_SIZE, -1, "", "", "", "");
#endif
			return SayTextPrint(msg_text, MSG_BUF_SIZE, -1, sstr1, sstr2, sstr3, sstr4);
			break;
		}
		}
	}

	return 0;
}

int KB_ConvertString(char *in, char **ppout)
{
	char sz[4096];
	char binding[64];
	char *p;
	char *pOut;
	char *pEnd;
	const char *pBinding;

	if (!ppout)
		return 0;

	*ppout = NULL;
	p = in;
	pOut = sz;

	while (*p)
	{
		if (*p == '+')
		{
			pEnd = binding;

			while (*p && (isalnum(*p) || (pEnd == binding)) && ((pEnd - binding) < 63))
				*pEnd++ = *p++;

			*pEnd = '\0';
			pBinding = NULL;

			if (strlen(binding + 1) > 0)
				pBinding = gEngfuncs.Key_LookupBinding(binding + 1);

			if (pBinding)
			{
				*pOut++ = '[';
				pEnd = (char *)pBinding;
			}
			else
				pEnd = binding;

			while (*pEnd)
				*pOut++ = *pEnd++;

			if (pBinding)
				*pOut++ = ']';
		}
		else
			*pOut++ = *p++;
	}

	*pOut = '\0';
	pOut = (char *)malloc(strlen(sz) + 1);
	strcpy(pOut, sz);
	*ppout = pOut;

	return 1;
}

int CHudMessage::MessageAdd(client_textmessage_t *newMessage, float time, int hintMessage, int useSlot, unsigned int m_hFont, bool bIsDynamicMessage)
{
	int i;
	client_textmessage_t *tempMessage;
	int emptySlot = useSlot;

	if (emptySlot == -1)
	{
		for (i = 0; i < maxHUDMessages; i++)
		{
			if (m_pMessages[i].pMessage)
			{
				if (m_pMessages[i].hintMessage & hintMessage)
					return -1;
			}
			else
			{
				if (emptySlot == -1)
					emptySlot = i;
			}
		}
	}

	if (emptySlot == -1)
		return -1;

	tempMessage = newMessage;

	if (hintMessage)
	{
		tempMessage->effect = 2;
		tempMessage->r1 = 40;
		tempMessage->g1 = 255;
		tempMessage->b1 = 40;
		tempMessage->a1 = 200;
		tempMessage->r2 = 0;
		tempMessage->g2 = 255;
		tempMessage->b2 = 0;
		tempMessage->a2 = 200;
		tempMessage->x = -1;
		tempMessage->y = 0.7;
		tempMessage->fadein = 0.01;
		tempMessage->fadeout = 0.7;
		tempMessage->fxtime = 0.07;
		tempMessage->holdtime = 5;

		if (!strcmp(tempMessage->pName, "#Spec_Duck"))
		{
			tempMessage->holdtime = 6;
		}

		if (tempMessage->pMessage && tempMessage->pMessage[0])
		{
			tempMessage->holdtime = strlen(tempMessage->pMessage) / 25;

			if (tempMessage->holdtime < 1)
				tempMessage->holdtime = 1;
		}
	}

	m_pMessages[emptySlot].pMessage = tempMessage;
	m_pMessages[emptySlot].font = m_hFont;
	m_pMessages[emptySlot].hintMessage = hintMessage;
	m_startTime[emptySlot] = time;

	if (bIsDynamicMessage)
		m_DynamicTextMessages.emplace_back(newMessage);

	return emptySlot;
}

void CHudMessage::MessageAdd(client_textmessage_t *newMessage, bool bIsDynamicMessage)
{
	m_parms.time = (*cl_time);

	for (int i = 0; i < maxHUDMessages; i++)
	{
		if (!m_pMessages[i].pMessage)
		{
			m_pMessages[i].pMessage = newMessage;
			m_pMessages[i].font = m_hFont;
			m_startTime[i] = (*cl_time);
			return;
		}
	}

	if (bIsDynamicMessage)
		m_DynamicTextMessages.emplace_back(newMessage);
}

client_textmessage_t *pfnTextMessageGet(const char *pName)
{
	if (g_pViewPort)
	{
		CDictionary *dict = g_pViewPort->FindDictionary(pName);

		if (dict)
		{
			return NULL;
		}
	}

	return gPrivateFuncs.pfnTextMessageGet(pName);
}

int __MsgFunc_ShowMenu(const char* pszName, int iSize, void* pbuf)
{
	if (m_HudMenu.MsgFunc_ShowMenu(pszName, iSize, pbuf) != 0)
		return 1;

	int result = m_pfnShowMenu(pszName, iSize, pbuf);

	return result;
}

void CHudMenu::Init(void)
{
	m_pfnShowMenu = HOOK_MESSAGE(ShowMenu);

	m_bMenuDisplayed = false;
	m_bitsValidSlots = 0;

	Reset();
}

void CHudMenu::Reset(void)
{
	m_szMenuString[0] = 0;
	m_szPrelocalisedMenuString[0] = 0;
	m_fWaitingForMore = 0;
	m_flShutoffTime = 0;
}

int CHudMenu::VidInit(void)
{
	IScheme *pScheme = scheme()->GetIScheme(scheme()->GetScheme("CaptionScheme"));

	if (pScheme)
	{
		m_hFont = pScheme->GetFont("Legacy_CreditsFont", true);
		if (!m_hFont)
			m_hFont = pScheme->GetFont("CreditsFont", true);
	}
	else
	{
		pScheme = scheme()->GetIScheme(scheme()->GetDefaultScheme());
		if (pScheme)
		{
			m_hFont = pScheme->GetFont("Legacy_CreditsFont", true);
			if (!m_hFont)
				m_hFont = pScheme->GetFont("CreditsFont", true);
		}
	}

	m_iFontEngineHeight = vgui::surface()->GetFontTall(m_hFont);

	Reset();

	return 1;
}

int CHudMenu::Draw(void)
{
	if (m_flShutoffTime > 0)
	{
		if (m_flShutoffTime <= (*cl_time))
		{
			m_bMenuDisplayed = false;
			return 1;
		}
	}

	if (g_pViewPort && g_pViewPort->IsScoreBoardVisible())
		return 1;

	if(!m_bMenuDisplayed)
		return 1;

	int nlc = 0;

	for (int i = 0; i < MAX_MENU_STRING && m_szMenuString[i] != '\0'; i++)
	{
		if (m_szMenuString[i] == '\n')
			nlc++;
	}

	int border = (gPrivateFuncs.CHud_GetBorderSize && gHud) ? gPrivateFuncs.CHud_GetBorderSize(gHud, 0) : 15;
	menu_x = border + 5;
	menu_r = 255;
	menu_g = 255;
	menu_b = 255;
	menu_ralign = 0;

	int ScreenWidth, ScreenHeight;

	SCREENINFO_s si; 
	si.iSize = sizeof(si);
	gEngfuncs.pfnGetScreenInfo(&si);

	ScreenWidth = si.iWidth;
	ScreenHeight = si.iHeight;

	int y = (ScreenHeight / 2) - ((nlc / 2) * 12) - 40;
	const char *sptr = m_szMenuString;
	int i;
	char menubuf[80];
	const char *ptr;

	while (*sptr)
	{
		if (*sptr == '\\')
		{
			switch (*(sptr + 1))
			{
			case '\0':
			{
				sptr += 1;
				break;
			}

			case 'w':
			{
				menu_r = 255;
				menu_g = 255;
				menu_b = 255;

				sptr += 2;
				break;
			}

			case 'd':
			{
				menu_r = 100;
				menu_g = 100;
				menu_b = 100;

				sptr += 2;
				break;
			}

			case 'y':
			{
				menu_r = 255;
				menu_g = 210;
				menu_b = 64;

				sptr += 2;
				break;
			}

			case 'r':
			{
				menu_r = 210;
				menu_g = 24;
				menu_b = 0;

				sptr += 2;
				break;
			}

			case 'R':
			{
				menu_ralign = 1;
				menu_x = ScreenWidth / 2 - border;

				sptr += 2;
				break;
			}

			default:
			{
				sptr += 2;
			}
			}

			continue;
		}

		if (*sptr == '\n')
		{
			menu_ralign = 0;
			menu_x = 20;
			y += m_iFontEngineHeight + 2;
			sptr += 1;
			continue;
		}

		for (ptr = sptr; *sptr != '\0'; sptr++)
		{
			if (*sptr == '\n')
				break;

			if (*sptr == '\\')
				break;
		}

		i = sptr - ptr;
		strncpy(menubuf, ptr, min(i, sizeof(menubuf)));
		menubuf[min(i, sizeof(menubuf) - 1)] = 0;

		if (menu_ralign)
		{
			menu_x = DrawHudStringReverse(menu_x, y, 0, menubuf, menu_r, menu_g, menu_b);
		}
		else
		{
			menu_x = DrawHudString(menu_x, y, 320, menubuf, menu_r, menu_g, menu_b);
		}
	}

	return 1;
}

int CHudMenu::DrawHudString(int xpos, int ypos, int iMaxX, char *szIt, int r, int g, int b)
{
	xpos += gEngfuncs.pfnDrawString(xpos, ypos, szIt, r, g, b);
	return xpos;
}

int CHudMenu::DrawHudStringReverse(int xpos, int ypos, int iMinX, char *szString, int r, int g, int b)
{
	xpos -= gEngfuncs.pfnDrawStringReverse(xpos, ypos, szString, r, g, b);
	return xpos;
}

char *CHudMenu::LocaliseTextString(const char *msg, char *dst_buffer, int buffer_size)
{
	char *dst = dst_buffer;

	for (char *src = (char *)msg; *src != 0 && buffer_size > 0; buffer_size--)
	{
		if (*src == '#')
		{
			static char word_buf[255];
			char *wdst = word_buf, *word_start = src;

			for (++src; (*src >= 'A' && *src <= 'z') || (*src >= '0' && *src <= '9'); wdst++, src++)
				*wdst = *src;

			*wdst = 0;

			client_textmessage_t *clmsg = gPrivateFuncs.pfnTextMessageGet(word_buf);

			if (!clmsg || !(clmsg->pMessage))
			{
				src = word_start;
				*dst = *src;
				dst++, src++;
				continue;
			}

			for (char *wsrc = (char *)clmsg->pMessage; *wsrc != 0; wsrc++, dst++)
				*dst = *wsrc;

			*dst = 0;
		}
		else
		{
			*dst = *src;
			dst++, src++;
			*dst = 0;
		}
	}

	dst_buffer[buffer_size - 1] = 0;
	return dst_buffer;
}

char *CHudMenu::BufferedLocaliseTextString(const char *msg)
{
	static char dst_buffer[1024];
	LocaliseTextString(msg, dst_buffer, 1024);
	return dst_buffer;
}

bool CHudMenu::SelectMenuItem(int menu_item)
{
	char szbuf[32];

	if ((menu_item > 0) && (m_bitsValidSlots & (1 << (menu_item - 1))))
	{
		if (m_bIsASMenu)
		{
			sprintf(szbuf, "as_menuselect %d\n", menu_item);
			gEngfuncs.pfnClientCmd(szbuf);
		}
		else
		{
			sprintf(szbuf, "menuselect %d\n", menu_item);
			gEngfuncs.pfnClientCmd(szbuf);
		}

		m_bMenuDisplayed = false;
		return true;
	}

	return false;
}

int CHudMenu::MsgFunc_ShowMenu(const char* pszName, int iSize, void* pbuf)
{
	if (g_bIsCounterStrike)
		return 0;

	if (!gPrivateFuncs.WeaponsResource_SelectSlot)
		return 0;

	char *temp = NULL;

	BEGIN_READ(pbuf, iSize);

	m_bitsValidSlots = READ_SHORT();

	int DisplayTime = READ_CHAR();
	int NeedMoreBits = READ_BYTE();
	int NeedMore = NeedMoreBits & 0x7F;
	m_bIsASMenu = ((NeedMoreBits >> 7) & 1) ? true : false;

	if (DisplayTime > 0)
		m_flShutoffTime = DisplayTime + (*cl_time);
	else
		m_flShutoffTime = -1;

	if (m_bitsValidSlots)
	{
		if (!m_fWaitingForMore)
			strncpy(m_szPrelocalisedMenuString, READ_STRING(), MAX_MENU_STRING);
		else
			strncat(m_szPrelocalisedMenuString, READ_STRING(), MAX_MENU_STRING - strlen(m_szPrelocalisedMenuString));

		m_szPrelocalisedMenuString[MAX_MENU_STRING - 1] = 0;

		if (!NeedMore)
		{
			strncpy(m_szMenuString, BufferedLocaliseTextString(m_szPrelocalisedMenuString), MAX_MENU_STRING - 1);
			m_szMenuString[MAX_MENU_STRING - 1] = 0;

			if (KB_ConvertString(m_szMenuString, &temp))
			{
				strcpy(m_szMenuString, temp);
				free(temp);
			}
		}

		m_bMenuDisplayed = true;
	}
	else
	{
		m_bMenuDisplayed = false;
	}

	m_fWaitingForMore = NeedMore;
	return 1;
}

void __fastcall WeaponsResource_SelectSlot(void *pthis, int, int iSlot, int fAdvance, int iDirection)
{
	if (m_HudMenu.m_bMenuDisplayed && (fAdvance == FALSE) && (iDirection == 1))
	{
		m_HudMenu.SelectMenuItem(iSlot + 1);
		return;
	}

	return gPrivateFuncs.WeaponsResource_SelectSlot(pthis, 0, iSlot, fAdvance, iDirection);
}