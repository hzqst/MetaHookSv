#include <metahook.h>
#include <cvardef.h>
#include "exportfuncs.h"
#include "privatehook.h"

cvar_t* cl_minmodels = NULL;
cvar_t* cl_min_t = NULL;
cvar_t* cl_min_ct = NULL;

extra_player_info_t(*g_PlayerExtraInfo)[65] = NULL;

bool BIsValidTModelIndex(int i)
{
	if (i == 5 || i == 1)
		return 1;
	if (i == 8 || i == 6)
		return 1;
	return i == 11;
}

bool BIsValidCTModelIndex(int i)
{
	if (i == 7 || i == 2)
		return 1;
	if (i == 10 || i == 4)
		return 1;
	return i == 9;
}

const char* sPlayerModelFiles[] = {
	"models/player.mdl",
	"models/player/leet/leet.mdl",
	"models/player/gign/gign.mdl",
	"models/player/vip/vip.mdl",
	"models/player/gsg9/gsg9.mdl",
	"models/player/guerilla/guerilla.mdl",
	"models/player/arctic/arctic.mdl",
	"models/player/sas/sas.mdl",
	"models/player/terror/terror.mdl",
	"models/player/urban/urban.mdl",
	"models/player/spetsnaz/spetsnaz.mdl",
	"models/player/militia/militia.mdl",
};

void CounterStrike_RedirectPlayerModelPath(const char* name, int PlayerID, int TeamID, char* pszModel, size_t cbModel)
{
	if (cl_minmodels && cl_minmodels->value && PlayerID > 0 && PlayerID < 65)
	{
		if (TeamID == 1)
		{
			if (!cl_min_t || !BIsValidTModelIndex((int)cl_min_t->value))
			{
				strncpy(pszModel, sPlayerModelFiles[1], cbModel - 1);
				pszModel[cbModel - 1] = 0;
			}
			else
			{
				strncpy(pszModel, sPlayerModelFiles[(int)cl_min_t->value], cbModel - 1);
				pszModel[cbModel - 1] = 0;
			}
		}
		else if (TeamID == 2)
		{
			if (!(*g_PlayerExtraInfo)[PlayerID].vip)
			{
				if (!cl_min_ct || !BIsValidCTModelIndex((int)cl_min_ct->value))
				{
					strncpy(pszModel, sPlayerModelFiles[2], cbModel - 1);
					pszModel[cbModel - 1] = 0;
				}
				else
				{
					strncpy(pszModel, sPlayerModelFiles[(int)cl_min_ct->value], cbModel - 1);
					pszModel[cbModel - 1] = 0;
				}
			}
			else
			{
				strncpy(pszModel, sPlayerModelFiles[3], cbModel - 1);
				pszModel[cbModel - 1] = 0;
			}
		}
	}

	if (!pszModel[0])
	{
		if (!strstr(name, "models/"))
		{
			snprintf(pszModel, cbModel - 1, "models/player/%s/%s.mdl", name, name);
			pszModel[cbModel - 1] = 0;
		}
		else
		{
			strncpy(pszModel, name, cbModel - 1);
			pszModel[cbModel - 1] = 0;
		}
	}
}

//For GameStudioRenderer
model_t* CounterStrike_RedirectPlayerModel(model_t* original_model, int PlayerNumber, int* modelindex)
{
	if (cl_minmodels && cl_minmodels->value && PlayerNumber > 0 && PlayerNumber < 65)
	{
		int TeamID = (*g_PlayerExtraInfo)[PlayerNumber].teamnumber;

		if (TeamID == 1)
		{
			if (!cl_min_t || !BIsValidTModelIndex((int)cl_min_t->value))
			{
				return gEngfuncs.CL_LoadModel(sPlayerModelFiles[1], modelindex);
			}
			else
			{
				return gEngfuncs.CL_LoadModel(sPlayerModelFiles[(int)cl_min_t->value], modelindex);
			}
		}
		else if (TeamID == 2)
		{
			if (!(*g_PlayerExtraInfo)[PlayerNumber].vip)
			{
				if (!cl_min_ct || !BIsValidCTModelIndex((int)cl_min_ct->value))
				{
					return gEngfuncs.CL_LoadModel(sPlayerModelFiles[2], modelindex);
				}
				else
				{
					return gEngfuncs.CL_LoadModel(sPlayerModelFiles[(int)cl_min_ct->value], modelindex);
				}
			}
			else
			{
				return gEngfuncs.CL_LoadModel(sPlayerModelFiles[3], modelindex);
			}
		}
	}

	return original_model;
}