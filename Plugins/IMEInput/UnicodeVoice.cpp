#include <metahook.h>
#include "Encode.h"

bool g_bUpdateVoiceState;
void (*g_pfnHUD_VoiceStatus)(int entindex, qboolean bTalking);

void HUD_VoiceStatus(int entindex, qboolean bTalking)
{
	g_bUpdateVoiceState = true;
	g_pfnHUD_VoiceStatus(entindex, bTalking);
	g_bUpdateVoiceState = false;
}

void GetPlayerInfo(int ent_num, hud_player_info_t *pinfo)
{
	gEngfuncs.pfnGetPlayerInfo(ent_num, pinfo);

	pinfo->name = "asdasdasdasd123";
	//if (!g_bUpdateVoiceState)
	//	return;
}