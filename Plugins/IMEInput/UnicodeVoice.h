extern void (*g_pfnHUD_VoiceStatus)(int entindex, qboolean bTalking);

void GetPlayerInfo(int ent_num, hud_player_info_t *pinfo);
void HUD_VoiceStatus(int entindex, qboolean bTalking);