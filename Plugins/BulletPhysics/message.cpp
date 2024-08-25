#include <metahook.h>
#include <cvardef.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "parsemsg.h"
#include "message.h"
#include "event_api.h"
#include "ClientEntityManager.h"
#include "CounterStrike.h"

bool g_bIsCreatingClCorpse = false;
int g_iCreatingClCorpsePlayerIndex = 0;

pfnUserMsgHook m_pfnClCorpse = NULL;

int __MsgFunc_ClCorpse(const char *pszName, int iSize, void *pbuf)
{
	vec3_t vOrigin, vAngles;

	BEGIN_READ(pbuf, iSize);
	auto model = READ_STRING();

	char szModel[64] = { 0 };
	strncpy(szModel, model, sizeof(szModel));

	vOrigin[0] = 0.0078125f * READ_LONG();
	vOrigin[1] = 0.0078125f * READ_LONG(); 
	vOrigin[2] = 0.0078125f * READ_LONG();
	vAngles[0] = READ_COORD();
	vAngles[1] = READ_COORD();
	vAngles[2] = READ_COORD();
	auto Delay = READ_LONG();
	auto Sequence = READ_BYTE();
	auto Body = READ_BYTE();
	auto TeamID = READ_BYTE();
	auto PlayerID = READ_BYTE();

	char szNewModel[64] = { 0 };
	CounterStrike_RedirectPlayerModelPath(szModel, PlayerID, TeamID, szNewModel, sizeof(szNewModel));

	g_bIsCreatingClCorpse = true;
	
	//TODO Find player with:
	//pev->effects |= EF_NOINTERP;
	//pev->framerate = 0;
	if (PlayerID <= 0 || PlayerID > gEngfuncs.GetMaxClients())
	{
		g_iCreatingClCorpsePlayerIndex = ClientEntityManager()->FindDyingPlayer(szModel, vOrigin, vAngles, Sequence, Body);
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