#pragma once

#include <memory>

class IShadowTexture
{
public:
	virtual ~IShadowTexture() {};

	virtual bool IsReady() const = 0;
	virtual void SetReady(bool bReady) = 0;

	virtual bool IsCascaded() const = 0;
	virtual bool IsStatic() const = 0;

	virtual GLuint GetDepthTexture() const = 0;
	virtual uint32_t GetTextureSize() const = 0;

	virtual void SetViewport(float x, float y, float w, float h) = 0;
	virtual const float* GetViewport() const = 0;

	virtual void SetWorldMatrix(int cascadedIndex, const mat4*mat) = 0;
	virtual void SetProjectionMatrix(int cascadedIndex, const mat4* mat) = 0;
	virtual void SetShadowMatrix(int cascadedIndex, const mat4* mat) = 0;

	virtual const mat4* GetWorldMatrix(int cascadedIndex) const = 0;
	virtual const mat4* GetProjectionMatrix(int cascadedIndex) const = 0;
	virtual const mat4* GetShadowMatrix(int cascadedIndex) const = 0;

	virtual void SetCSMDistance(int cascadedIndex, float distance) = 0;
	virtual float GetCSMDistance(int cascadedIndex) const = 0;
};

extern std::shared_ptr<IShadowTexture> g_DLightShadowTextures[MAX_DLIGHTS_SVENGINE];

extern int g_iCurrentShadowCascadedIndex;
extern std::shared_ptr<IShadowTexture> g_pCurrentShadowTexture;

//cvar
extern cvar_t *r_shadow;

bool R_ShouldCastShadow(cl_entity_t *ent);
bool R_ShouldRenderShadow(void);
void R_RenderShadowMap(void); 
void R_InitShadow(void);
void R_ShutdownShadow(void);
std::shared_ptr<IShadowTexture> R_CreateBaseShadowTexture(uint32_t size);