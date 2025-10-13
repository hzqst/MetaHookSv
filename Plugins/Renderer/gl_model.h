#pragma once

extern byte mod_novis[MAX_MAP_LEAFS_SVENGINE / 8];

void Mod_UnloadSpriteTextures(model_t* mod);
void Mod_LoadSpriteModel(model_t* mod, void* buffer);
void Mod_LoadStudioModel(model_t* mod, void* buffer);
void Mod_LoadBrushModel(model_t* mod, void* buffer);
void Mod_LeafPVS(mleaf_t* leaf, model_t* model, byte*decompressed);

class CGameModelRenderTexture
{
public:
	int gltexturenum{};
	int width{}, height{};
	float scaleX{ 1 }, scaleY{ 1 };
	int numframes{};
	float framerate{};
};

class CGameResourceAsyncLoadTask : public IBaseInterface
{
public:
	ThreadWorkItemHandle_t m_hThreadWorkItem{};
	std::atomic<bool> m_IsDataReady{};

	virtual void StartAsyncTask() {};
	virtual bool RunTask() { return false; };
	virtual void UploadResource() {};
};
