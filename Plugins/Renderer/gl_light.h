#pragma once

#include <memory>

enum DynamicLightType
{
	DynamicLightType_Unknown = 0,
	DynamicLightType_Point,
	DynamicLightType_Spot,
	DynamicLightType_Directional
};

class IShadowTexture;

class CDynamicLight
{
public:
	~CDynamicLight();

	DynamicLightType type{ DynamicLightType_Unknown };
	vec3_t origin{};
	vec3_t angles{};
	float size{}; // Orthographic projection width/height for DirectionalLight, or radius for point light
	float color[3]{};
	float distance{};//Spotlight range
	float ambient{};
	float diffuse{};
	float specular{};
	float specularpow{};
	int shadow{ 0 };
	int dynamic_shadow_size{ 0 };
	int static_shadow_size{};
	float csm_lambda{ 0.5f };
	float csm_margin{ 0.15f };
	int follow_player{ 0 };
	std::shared_ptr<IShadowTexture> pStaticShadowTexture;
	std::shared_ptr<IShadowTexture> pDynamicShadowTexture;
};

extern std::vector<std::shared_ptr<CDynamicLight>> g_DynamicLights;

extern cvar_t * r_deferred_lighting;

extern MapConVar* r_deferred_lightmap_pow;
extern MapConVar* r_deferred_lightmap_scale;

extern MapConVar* r_flashlight_enable;
extern MapConVar *r_flashlight_ambient;
extern MapConVar *r_flashlight_diffuse;
extern MapConVar *r_flashlight_specular;
extern MapConVar *r_flashlight_specularpow;
extern MapConVar *r_flashlight_attachment;
extern MapConVar *r_flashlight_distance;
extern MapConVar *r_flashlight_cone_cosine;
extern GLuint r_flashlight_cone_texture;
extern std::string r_flashlight_cone_texture_name;

extern MapConVar *r_dynlight_ambient;
extern MapConVar *r_dynlight_diffuse;
extern MapConVar *r_dynlight_specular;
extern MapConVar *r_dynlight_specularpow;

extern cvar_t *r_ssr;
extern MapConVar *r_ssr_ray_step;
extern MapConVar *r_ssr_iter_count;
extern MapConVar *r_ssr_distance_bias;
extern MapConVar *r_ssr_exponential_step;
extern MapConVar *r_ssr_adaptive_step;
extern MapConVar *r_ssr_binary_search;
extern MapConVar *r_ssr_fade;

extern bool r_draw_gbuffer;

typedef struct PointLightCallbackArgs_s
{
	float radius;
	vec3_t origin;
	vec3_t color;
	float ambient;
	float diffuse;
	float specular;
	float specularpow;

	std::shared_ptr<IShadowTexture>* ppStaticShadowTexture;
	std::shared_ptr<IShadowTexture>* ppDynamicShadowTexture;
	uint32_t dynamicShadowSize;
	uint32_t staticShadowSize;

	bool bVolume;
	bool bStatic;
}PointLightCallbackArgs;

typedef void(*fnPointLightCallback)(PointLightCallbackArgs *args, void *context);

typedef struct SpotLightCallbackArgs_s
{
	float distance;
	float radius;
	float coneAngle;
	float coneCosAngle;
	float coneSinAngle;
	float coneTanAngle;
	vec3_t origin;
	vec3_t angle;
	vec3_t vforward;
	vec3_t vright;
	vec3_t vup;
	vec3_t color;
	float ambient;
	float diffuse;
	float specular;
	float specularpow;

	std::shared_ptr<IShadowTexture>* ppStaticShadowTexture;
	std::shared_ptr<IShadowTexture>* ppDynamicShadowTexture;
	uint32_t dynamicShadowSize;
	uint32_t staticShadowSize;

	bool bVolume;
	bool bStatic;
	bool bHideEntitySource;
	cl_entity_t* pHideEntity;
}SpotLightCallbackArgs;

typedef void(*fnSpotLightCallback)(SpotLightCallbackArgs *args, void *context);

typedef struct DirectionalLightCallbackArgs_s
{
	vec3_t origin;
	vec3_t angle;
	vec3_t vforward;
	vec3_t vright;
	vec3_t vup;
	float size;
	vec3_t color;
	float ambient;
	float diffuse;
	float specular;
	float specularpow;

	std::shared_ptr<IShadowTexture>* ppStaticShadowTexture;
	std::shared_ptr<IShadowTexture>* ppDynamicShadowTexture;
	uint32_t dynamicShadowSize;
	uint32_t staticShadowSize;
	float csmLambda;//0.5 by default
	float csmMargin;//0.15 by default

	bool bVolume;
	bool bStatic;
}DirectionalLightCallbackArgs;

typedef void(*fnDirectionalLightCallback)(DirectionalLightCallbackArgs* args, void* context);

void R_IterateDynamicLights(
	fnPointLightCallback pointlightCallback,
	fnSpotLightCallback spotlightCallback,
	fnDirectionalLightCallback directionalLightCallback,
	void* context);

typedef struct
{
	int program;
	int u_lightdir;
	int u_lightright;
	int u_lightup;
	int u_lightpos;
	int u_lightcolor;
	int u_lightcone;
	int u_lightradius;
	int u_lightambient;
	int u_lightdiffuse;
	int u_lightspecular;
	int u_lightspecularpow;
	int u_lightSize;
	int u_modelmatrix;
	int u_staticShadowTexel;
	int u_staticShadowMatrix;
	int u_dynamicShadowTexel;
	int u_dynamicShadowMatrix;
	int u_staticCubemapShadowTexel;
	int u_dynamicCubemapShadowTexel;
	int u_csmMatrices;
	int u_csmDistances;
	int u_csmTexel;
}dlight_program_t;

typedef struct
{
	int program;
	int u_ssrRayStep;
	int u_ssrIterCount;
	int u_ssrDistanceBias;
	int u_ssrFade;
}dfinal_program_t;

void R_LoadLightResources(void);
void R_InitLight(void);
void R_ShutdownLight(void);
bool R_CanRenderGBuffer(void);
void R_BeginRenderGBuffer(void);
void R_EndRenderGBuffer(FBO_Container_t* dst);
void R_SetGBufferMask(int mask);
void R_SetGBufferBlend(int blendsrc, int blenddst);
void R_SaveDLightProgramStates(void);
void R_SaveDFinalProgramStates(void);
void R_LoadDLightProgramStates(void);
void R_LoadDFinalProgramStates(void);
void R_BlitGBufferToFrameBuffer(FBO_Container_t* fbo, bool color, bool depth, bool stencil);
GLuint R_GetEmptyVAO();