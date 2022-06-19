#pragma once

extern bool g_bIsCreatingClCorpse;
extern int g_iCreatingClCorpsePlayerIndex;

extern pfnUserMsgHook m_pfnClCorpse;

int __MsgFunc_ClCorpse(const char *pszName, int iSize, void *pbuf);