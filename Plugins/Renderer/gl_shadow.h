#pragma once

#include <memory>

class IShadowTexture
{
public:
	virtual ~IShadowTexture() {};

	virtual bool IsReady() const = 0;
	virtual void SetReady(bool bReady) = 0;

	/*
		Single Layer Shadowmap
	*/
	virtual bool IsSingleLayer() const = 0;

	/*
		Directional light CSM
	*/
	virtual bool IsCascaded() const = 0;

	/*
		Point light omnidirectional shadow maps
	*/
	virtual bool IsCubemap() const = 0;

	/*
		Static shadow texture that won't change once baked with any poly onto it
	*/
	virtual bool IsStatic() const = 0;

	/*
		OpenGL texture id
	*/
	virtual GLuint GetDepthTexture() const = 0;

	/*
		OpenGL texture size
	*/
	virtual uint32_t GetTextureSize() const = 0;

	virtual void SetViewport(float x, float y, float w, float h) = 0;
	virtual const float* GetViewport() const = 0;

	/*
		index can be cascadedIndex for CSM, or cubemapIndex for cubemap shadow mapping
	*/
	virtual void SetWorldMatrix(int index, const mat4*mat) = 0;
	virtual void SetProjectionMatrix(int index, const mat4* mat) = 0;
	virtual void SetShadowMatrix(int index, const mat4* mat) = 0;

	virtual const mat4* GetWorldMatrix(int index) const = 0;
	virtual const mat4* GetProjectionMatrix(int index) const = 0;
	virtual const mat4* GetShadowMatrix(int index) const = 0;

	virtual void SetCSMDistance(int index, float distance) = 0;
	virtual float GetCSMDistance(int index) const = 0;
};

extern std::shared_ptr<IShadowTexture> g_DLightShadowTextures[MAX_DLIGHTS_SVENGINE];

extern std::shared_ptr<IShadowTexture> g_pCurrentShadowTexture;

//cvar
extern cvar_t *r_shadow;

bool R_ShouldCastShadow(cl_entity_t *ent);
bool R_ShouldRenderShadow(void);
void R_RenderShadowMap(void); 
void R_InitShadow(void);
void R_ShutdownShadow(void);
std::shared_ptr<IShadowTexture> R_CreateBaseShadowTexture(uint32_t size);