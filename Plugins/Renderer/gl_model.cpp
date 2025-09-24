#include "gl_local.h"

byte mod_novis[MAX_MAP_LEAFS_SVENGINE / 8] = {0};

int* gSpriteMipMap = NULL;

int LittleLong(int l)
{
	return l;
}

short LittleShort(short l)
{
	return l;
}

float LittleFloat(float l)
{
	return l;
}

void CM_DecompressPVS(byte* in, byte* decompressed, int byteCount)
{
	int		c;
	byte* out;

	out = decompressed;

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out < decompressed + byteCount);
}

void Mod_DecompressVis(byte* in, model_t* model, byte* decompressed)
{
	int row = (model->numleafs + 7) >> 3;

	CM_DecompressPVS(in, decompressed, row);
}

void Mod_LeafPVS(mleaf_t* leaf, model_t* model, byte *decompressed)
{
	if (!leaf || leaf == model->leafs || !leaf->compressed_vis) {
		memcpy(decompressed, mod_novis, MAX_MAP_LEAFS_SVENGINE / 8);
		return;
	}

	Mod_DecompressVis(leaf->compressed_vis, model, decompressed);
}

void Mod_Init(void)
{
	memset(mod_novis, 0xff, sizeof(mod_novis));
}

void Mod_UnloadSpriteTextures(model_t* mod)
{
	if (mod->type != mod_sprite)
		return;

	auto pSprite = (msprite_t *)mod->cache.data;

	mod->needload = NL_NEEDS_LOADED;

	if (!pSprite)
		return;

	for (int i = 0; i < pSprite->numframes; i++)
	{
		char name[260] = {0};
		snprintf(name, sizeof(name), "%s_%i", mod->name, i);

		GL_UnloadTextureWithType(name, GLT_SPRITE);
		GL_UnloadTextureWithType(name, GLT_HUDSPRITE);
	}
}

void Mod_LoadBrushModel(model_t* mod, void* buffer)
{
	gPrivateFuncs.Mod_LoadBrushModel(mod, buffer);

	//TODO
}

void Mod_LoadStudioModel(model_t* mod, void* buffer)
{
	gPrivateFuncs.Mod_LoadStudioModel(mod, buffer);

	if (mod->needload == NL_UNREFERENCED && mod->cache.data)
	{
		mod->needload = NL_PRESENT;
	}

	auto studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);

	if (studiohdr)
	{
		if ((int)r_studio_lazy_load->value == 0)
		{
			//Force load
			R_CreateStudioRenderData(mod, studiohdr);
		}
		else
		{
			//Only reload if already present
			auto pRenderData = R_GetStudioRenderDataFromModel(mod);

			if (pRenderData)
			{
				R_CreateStudioRenderData(mod, studiohdr);
			}
		}
	}
}

void Mod_LoadSpriteModel(model_t* mod, void* buffer)
{
	gPrivateFuncs.Mod_LoadSpriteModel(mod, buffer);

	R_CreateSpriteRenderData(mod);
}
