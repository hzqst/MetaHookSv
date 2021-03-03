#include <metahook.h>
#include "exportfuncs.h"
#include "engfuncs.h"
#include "parsemsg.h"
//viewport
#include <VGUI/VGUI.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include "Viewport.h"
#include "message.h"
#include "msghook.h"
#include <algorithm>

using namespace vgui;

CHudMessage m_HudMessage;

//2015-11-27 added, support for Counter-Strike's HudTextPro message
pfnUserMsgHook m_pfnHudText;
pfnUserMsgHook m_pfnHudTextPro;
pfnUserMsgHook m_pfnHudTextArgs;

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

void CHudMessage::Init(void)
{
	m_pfnHudText = HOOK_MESSAGE(HudText);
	m_pfnHudTextPro = HOOK_MESSAGE(HudTextPro);
	m_pfnHudTextArgs = HOOK_MESSAGE(HudTextArgs);
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

		if (pClientMessage->numArgs)
		{
			vgui::localize()->ConstructString(textBuf, sizeof(textBuf), pFullText, pClientMessage->numArgs, pClientMessage->args[0], pClientMessage->args[1], pClientMessage->args[2], pClientMessage->args[3]);
			pFullText = textBuf;
		}

		if (!pFullText)
		{
			gEngfuncs.Con_DPrintf("ERROR: Missing %s from the dictionary_english.txt file!\n", pMessage->pMessage);
			return;
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

int CHudMessage::Draw(void)
{
	int i;
	client_textmessage_t *pMessage;
	float endTime;

	float fTime = cl_time;

	for (i = 0; i < maxHUDMessages; i++)
	{
		if (m_pMessages[i].pMessage)
		{
			pMessage = m_pMessages[i].pMessage;

			if (m_startTime[i] > cl_time || m_startTime[i] == 1.0)
				m_startTime[i] = cl_time + m_parms.time - m_startTime[i] + 0.2;
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
				m_pMessages[i].pMessage = NULL;
				m_pMessages[i].font = 0;
			}
		}
	}

	m_parms.time = cl_time;

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
			int useSlot = -1;
			char *slotString = &pString[sizeof("__NETMESSAGE__") - 1];
			if (isdigit(*slotString))
			{
				useSlot = atoi(slotString);
			}

			client_textmessage_t *pTextMessage = NULL;

			if (pString[0] == '#')
				pTextMessage = pfnTextMessageGet(pString + 1);
			else
				pTextMessage = pfnTextMessageGet(pString);

			std::smatch result;
			std::string str = pTextMessage->pMessage;

			dict = g_pViewPort->FindDictionary(str.c_str(), DICT_NETMESSAGE);
			
			if(!dict)
				dict = g_pViewPort->FindDictionaryRegex(str, DICT_NETMESSAGE, result);

			if (cap_debug && cap_debug->value)
			{
				gEngfuncs.Con_Printf((dict) ? "CaptionMod: NetMessage [%s] found.\n" : "CaptionMod: NetMessage [%s] not found.\n", pTextMessage->pMessage);
			}

			if (dict && dict->m_bRegex && result.size() > 1)
			{
				if (!dict->m_pTextMessage)
				{
					dict->m_pTextMessage = new client_textmessage_t;
					memcpy(dict->m_pTextMessage, pTextMessage, sizeof(*pTextMessage));
					dict->m_pTextMessage->pMessage = (const char *)new char[HUDMESSAGE_MAXLENGTH];
				}

				std::string sentence;
				sentence.resize(HUDMESSAGE_MAXLENGTH);

				int finalLength = localize()->ConvertUnicodeToANSI(dict->m_szSentence.Base(), (char *)sentence.data(), sentence.length());

				sentence.resize(finalLength);

				for (size_t i = 1; i < result.size(); ++i)
				{
					char temp[32] = {0};
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

				V_strncpy((char *)dict->m_pTextMessage->pMessage, sentence.data(), HUDMESSAGE_MAXLENGTH - 1);
				((char *)dict->m_pTextMessage->pMessage)[HUDMESSAGE_MAXLENGTH - 1] = 0;

				MessageAdd(dict->m_pTextMessage, cl_time, hintMessage, useSlot, m_hFont);
				g_pViewPort->StartNextSubtitle(dict);
				m_parms.time = cl_time;
				return 1;
			}
			else if (dict && !dict->m_bRegex)
			{
				if (!dict->m_pTextMessage)
				{
					dict->m_pTextMessage = new client_textmessage_t;
					memcpy(dict->m_pTextMessage, pTextMessage, sizeof(*pTextMessage));
					dict->m_pTextMessage->pMessage = (const char *)new char[HUDMESSAGE_MAXLENGTH];
				}

				char sentence[HUDMESSAGE_MAXLENGTH];

				int finalLength = localize()->ConvertUnicodeToANSI(dict->m_szSentence.Base(), (char *)sentence, HUDMESSAGE_MAXLENGTH);

				sentence[finalLength] = 0;

				V_strncpy((char *)dict->m_pTextMessage->pMessage, sentence, HUDMESSAGE_MAXLENGTH - 1);
				((char *)dict->m_pTextMessage->pMessage)[HUDMESSAGE_MAXLENGTH - 1] = 0;

				MessageAdd(dict->m_pTextMessage, cl_time, hintMessage, useSlot, m_hFont);
				g_pViewPort->StartNextSubtitle(dict);
				m_parms.time = cl_time;
				return 1;
			}
			else if(pTextMessage)
			{
				MessageAdd(pTextMessage, cl_time, hintMessage, useSlot, m_hFont);
				m_parms.time = cl_time;
				return 1;
			}
		}

		return 0;
	}
	else
	{
		dict = g_pViewPort->FindDictionary(pString, DICT_MESSAGE);

		if (cap_debug && cap_debug->value)
		{
			gEngfuncs.Con_Printf((dict) ? "CaptionMod: TextMessage [%s] found.\n" : "CaptionMod: TextMessage [%s] not found.\n", pString);
		}

		if (dict)
		{
			MessageAdd(dict->m_pTextMessage, cl_time, hintMessage, -1, m_hFont);
			m_parms.time = cl_time;
			return 1;
		}
	}
	
	return 0;
}

int CHudMessage::MsgFunc_HudTextArgs(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	char *pString = READ_STRING();
	int hintMessage = READ_BYTE();

	CDictionary *dict = g_pViewPort->FindDictionary(pString, DICT_MESSAGE);

	if(cap_debug && cap_debug->value)
	{
		gEngfuncs.Con_Printf((dict) ? "CaptionMod: TextMessage [%s] found.\n" : "CaptionMod: TextMessage [%s] not found.\n", pString);
	}

	if(dict && dict->m_pTextMessage)
	{
		dict->ReplaceKey();

		int slotNum = MessageAdd(dict->m_pTextMessage, cl_time, hintMessage, -1, m_hFont);

		if (slotNum > -1)
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

		g_pViewPort->StartNextSubtitle(dict);

		m_parms.time = cl_time;
		return 1;
	}

	return 0;
}

int CHudMessage::MessageAdd(client_textmessage_t *newMessage, float time, int hintMessage, int useSlot, unsigned int m_hFont)
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

		if (tempMessage->pMessage)
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
	return emptySlot;
}

void CHudMessage::MessageAdd(client_textmessage_t *newMessage)
{
	m_parms.time = cl_time;

	for (int i = 0; i < maxHUDMessages; i++)
	{
		if (!m_pMessages[i].pMessage)
		{
			m_pMessages[i].pMessage = newMessage;
			m_pMessages[i].font = m_hFont;
			m_startTime[i] = cl_time;
			return;
		}
	}
}

client_textmessage_t *pfnTextMessageGet(const char *pName)
{
	if (g_pViewPort)
	{
		CDictionary *dict = g_pViewPort->FindDictionary(pName);

		if (dict && dict->m_pTextMessage)
		{
			return NULL;
		}
	}

	return gEngfuncs.pfnTextMessageGet(pName);
}