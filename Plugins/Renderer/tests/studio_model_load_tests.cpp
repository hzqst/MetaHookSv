// Exercise the production load hook with engine and RenderData callbacks stubbed.
#include "../gl_model.cpp"
#include <cstdio>

private_funcs_t gPrivateFuncs{};
cl_enginefunc_t gEngfuncs{};
engine_studio_api_t IEngineStudio{};
int g_iEngineType{};
static cvar_t lazyLoad{};
cvar_t* r_studio_lazy_load = &lazyLoad;

static studiohdr_t bodyHeader{}, errorHeader{};
static model_t textureModel{};
static int textureLoads{}, bodyReloads{}, renderDataCreates{};
static bool expectFallback{};
static std::shared_ptr<CStudioModelRenderData> cachedRenderData;

CStudioModelRenderData::~CStudioModelRenderData() = default;
StudioConVar::StudioConVar() = default;
StudioConVar::~StudioConVar() = default;

static void ConsolePrint(const char*, ...) {}

void GL_UnloadTextureWithType(const char*, GL_TEXTURETYPE)
{
	assert(false);
}

std::shared_ptr<CStudioModelRenderData> R_GetStudioRenderDataFromModel(model_t*)
{
	return cachedRenderData;
}

std::shared_ptr<CStudioModelRenderData> R_CreateStudioRenderData(model_t* mod, studiohdr_t* hdr)
{
	++renderDataCreates;
	assert((expectFallback ? &errorHeader : &bodyHeader) == hdr);
	assert(mod->cache.data == hdr);
	return nullptr;
}

std::shared_ptr<CSpriteModelRenderData> R_CreateSpriteRenderData(model_t*)
{
	return nullptr;
}

model_t* R_StudioLoadTextureModel(model_t*)
{
	++textureLoads;
	return &textureModel;
}

static void EngineLoadStudioModel(model_t* mod, void* buffer)
{
	mod->type = mod_studio;
	mod->cache.data = buffer;
}

static model_t* EngineReloadModel(model_t* mod, qboolean crash, qboolean trackCRC)
{
	++bodyReloads;
	assert(0 == strcmp("models/error.mdl", mod->name));
	assert(NL_NEEDS_LOADED == mod->needload);
	assert(nullptr == mod->texinfo);
	assert(crash && !trackCRC);
	mod->needload = NL_PRESENT;
	Mod_LoadStudioModel(mod, &errorHeader);
	return mod;
}

static void TestLoad(int engineType, bool missingTexture, bool embeddedTexture, bool lazy)
{
	model_t model{};
	strcpy(model.name, "models/test.mdl");
	model.needload = NL_PRESENT;
	bodyHeader = {};
	errorHeader = {};
	bodyHeader.textureindex = embeddedTexture ? sizeof(studiohdr_t) : 0;
	errorHeader.textureindex = sizeof(studiohdr_t);
	textureModel = {};
	strcpy(textureModel.name, missingTexture ? "models/error.mdl" : "models/testT.mdl");
	textureLoads = bodyReloads = renderDataCreates = 0;
	g_iEngineType = engineType;
	lazyLoad.value = lazy ? 1.0f : 0.0f;
	expectFallback = engineType == ENGINE_SVENGINE && missingTexture && !embeddedTexture;

	Mod_LoadStudioModel(&model, &bodyHeader);

	assert((expectFallback ? 1 : 0) == bodyReloads);
	assert((engineType == ENGINE_SVENGINE && !embeddedTexture ? 1 : 0) == textureLoads);
	assert((lazy && !cachedRenderData ? 0 : 1) == renderDataCreates);
	assert((expectFallback ? &errorHeader : &bodyHeader) == model.cache.data);
	assert(0 == strcmp(expectFallback ? "models/error.mdl" : "models/test.mdl", model.name));
	if (engineType == ENGINE_SVENGINE && !embeddedTexture && !missingTexture)
		assert((mtexinfo_t*)&textureModel == model.texinfo);
}

int main()
{
	gPrivateFuncs.Mod_LoadStudioModel = EngineLoadStudioModel;
	gPrivateFuncs.Mod_LoadModel = EngineReloadModel;
	IEngineStudio.Mod_Extradata = [](model_t* mod) -> void* { return mod->cache.data; };
	gEngfuncs.Con_Printf = ConsolePrint;
	for (bool cached : { false, true })
	{
		cachedRenderData = cached ? std::make_shared<CStudioModelRenderData>(nullptr) : nullptr;
		for (bool lazy : { false, true })
		{
			TestLoad(ENGINE_SVENGINE, true, false, lazy);
			TestLoad(ENGINE_SVENGINE, false, false, lazy);
			TestLoad(ENGINE_SVENGINE, true, true, lazy);
			TestLoad(ENGINE_GOLDSRC, true, false, lazy);
			TestLoad(ENGINE_GOLDSRC_HL25, true, false, lazy);
		}
	}
	std::puts("Studio model load tests passed (20 cases).");
}
