#include <metahook.h>
#include "exportfuncs.h"
#include "parsemsg.h"
#include "message.h"
#include "corpse.h"
#include "event_api.h"

bool g_bIsCreatingClCorpse = false;
int g_iCreatingClCorpsePlayerIndex = 0;

pfnUserMsgHook m_pfnClCorpse;

int __MsgFunc_ClCorpse(const char *pszName, int iSize, void *pbuf)
{
	vec3_t vOrigin, vAngles;

	BEGIN_READ(pbuf, iSize);
	auto name = READ_STRING();

	char szModel[64] = {0};

	if (!strstr(name, "models/"))
	{
		snprintf(szModel, 64, "models/player/%s/%s.mdl", name, name);
		szModel[63] = 0;
	}
	else
	{
		strncpy(szModel, name, 64);
		szModel[63] = 0;
	}

	vOrigin[0] = 0.0078125f * READ_LONG();
	vOrigin[1] = 0.0078125f * READ_LONG(); 
	vOrigin[2] = 0.0078125f * READ_LONG();
	vAngles[0] = READ_COORD();
	vAngles[1] = READ_COORD();
	vAngles[2] = READ_COORD();
	auto delay = READ_LONG();
	auto Sequence = READ_BYTE();
	auto Body = READ_BYTE();
	auto TeamID = READ_BYTE();
	auto PlayerID = READ_BYTE();

	g_bIsCreatingClCorpse = true;
	
	if (PlayerID <= 0 || PlayerID > gEngfuncs.GetMaxClients())
	{
		g_iCreatingClCorpsePlayerIndex = gCorpseManager.FindDyingPlayer(szModel, vOrigin, vAngles, Sequence, Body);
	}
	else
	{
		g_iCreatingClCorpsePlayerIndex = PlayerID;
	}

	auto r = m_pfnClCorpse(pszName, iSize, pbuf);

	g_iCreatingClCorpsePlayerIndex = 0;
	g_bIsCreatingClCorpse = false;

	return r;
}