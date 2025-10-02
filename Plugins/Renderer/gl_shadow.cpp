#include "gl_local.h"
#include <sstream>
#include <math.h>

std::shared_ptr<IShadowTexture> g_DLightShadowTextures[MAX_DLIGHTS_SVENGINE]{};

std::shared_ptr<IShadowTexture> g_pCurrentShadowTexture{};

//cvar
cvar_t* r_shadow = NULL;

class CBaseShadowTexture : public IShadowTexture
{
public:
	CBaseShadowTexture(uint32_t size, bool bStatic) : m_size(size), m_bStatic(bStatic)
	{
		
	}

	~CBaseShadowTexture()
	{
		if (m_depthtex)
		{
			gEngfuncs.Con_DPrintf("CBaseShadowTexture: delete m_depthtex [%d].\n", m_depthtex);
			GL_DeleteTexture(m_depthtex);
			m_depthtex = 0;
		}
	}

	bool IsReady() const override
	{
		return m_ready;
	}

	void SetReady(bool bReady) override
	{
		m_ready = bReady;
	}

	bool IsCascaded() const override
	{
		return false;
	}

	bool IsCubemap() const override
	{
		return false;
	}

	bool IsStatic() const override
	{
		return m_bStatic;
	}
	
	GLuint GetDepthTexture() const override
	{
		return m_depthtex;
	}

	uint32_t GetTextureSize() const override
	{
		return m_size;
	}

	void SetViewport(float x, float y, float w, float h) override
	{
		m_viewport[0] = x;
		m_viewport[1] = y;
		m_viewport[2] = w;
		m_viewport[3] = h;
	}
	const float* GetViewport() const override
	{
		return m_viewport;
	}

	void SetCSMDistance(int cascadedIndex, float distance) override
	{

	}

	float GetCSMDistance(int cascadedIndex) const override
	{
		return 0;
	}

protected:
	GLuint m_depthtex{};
	uint32_t m_size{};
	float m_viewport[4]{};
	bool m_ready{};
	bool m_bStatic{};
};

class CSingleShadowTexture : public CBaseShadowTexture
{
public:
	CSingleShadowTexture(uint32_t size, bool bStatic) : CBaseShadowTexture(size, bStatic)
	{
		m_depthtex = GL_GenShadowTexture(size, size, true);
	}

	bool IsSingleLayer() const override
	{
		return true;
	}

	void SetWorldMatrix(int index, const mat4* mat) override
	{
		memcpy(m_worldmatrix, mat, sizeof(mat4));
	}
	void SetProjectionMatrix(int index, const mat4* mat) override
	{
		memcpy(m_projmatrix, mat, sizeof(mat4));
	}
	void SetShadowMatrix(int index, const mat4* mat) override
	{
		memcpy(m_shadowmatrix, mat, sizeof(mat4));
	}

	const mat4* GetWorldMatrix(int index) const override
	{
		return &m_worldmatrix;
	}
	const mat4* GetProjectionMatrix(int index) const override
	{
		return &m_projmatrix;
	}
	const mat4* GetShadowMatrix(int index) const override
	{
		return &m_shadowmatrix;
	}
private:
	mat4 m_worldmatrix{};
	mat4 m_projmatrix{};
	mat4 m_shadowmatrix{};
};

class CCascadedShadowTexture : public CBaseShadowTexture
{
public:
	CCascadedShadowTexture(uint32_t size, bool bStatic) : CBaseShadowTexture(size, bStatic)
	{
		// Use texture array for CSM: size x size x 4 layers
		m_depthtex = GL_GenShadowTextureArray(size, size, CSM_LEVELS, true);
	}

	bool IsCascaded() const override
	{
		return true;
	}

	bool IsSingleLayer() const override
	{
		return false;
	}

	void SetWorldMatrix(int index, const mat4* mat) override
	{
		memcpy(&m_worldmatrix[index], mat, sizeof(mat4));
	}
	void SetProjectionMatrix(int index, const mat4* mat) override
	{
		memcpy(&m_projmatrix[index], mat, sizeof(mat4));
	}
	void SetShadowMatrix(int index, const mat4* mat) override
	{
		memcpy(&m_shadowmatrix[index], mat, sizeof(mat4));
	}

	const mat4* GetWorldMatrix(int index) const override
	{
		return &m_worldmatrix[index];
	}
	const mat4* GetProjectionMatrix(int index) const override
	{
		return &m_projmatrix[index];
	}
	const mat4* GetShadowMatrix(int index) const override
	{
		return &m_shadowmatrix[index];
	}

	void SetCSMDistance(int index, float distance) override
	{
		m_csmDistances[index] = distance;
	}

	float GetCSMDistance(int index) const override
	{
		return m_csmDistances[index];
	}

private:
	float m_csmDistances[CSM_LEVELS]{};
	mat4 m_worldmatrix[CSM_LEVELS]{};
	mat4 m_projmatrix[CSM_LEVELS]{};
	mat4 m_shadowmatrix[CSM_LEVELS]{};
};

class CCubemapShadowTexture : public CBaseShadowTexture
{
public:
	CCubemapShadowTexture(uint32_t size, bool bStatic) : CBaseShadowTexture(size, bStatic)
	{
		m_depthtex = GL_GenCubemapShadowTexture(size, size, true);
	}

	bool IsCubemap() const override
	{
		return true;
	}

	bool IsSingleLayer() const override
	{
		return false;
	}

	void SetWorldMatrix(int index, const mat4* mat) override
	{
		memcpy(&m_worldmatrix[index], mat, sizeof(mat4));
	}
	void SetProjectionMatrix(int index, const mat4* mat) override
	{
		memcpy(&m_projmatrix[index], mat, sizeof(mat4));
	}
	void SetShadowMatrix(int index, const mat4* mat) override
	{
		memcpy(&m_shadowmatrix[index], mat, sizeof(mat4));
	}

	const mat4* GetWorldMatrix(int index) const override
	{
		return &m_worldmatrix[index];
	}
	const mat4* GetProjectionMatrix(int index) const override
	{
		return &m_projmatrix[index];
	}
	const mat4* GetShadowMatrix(int index) const override
	{
		return &m_shadowmatrix[index];
	}

private:
	mat4 m_worldmatrix[6]{};
	mat4 m_projmatrix[6]{};
	mat4 m_shadowmatrix[6]{};
};

int StudioGetSequenceActivityType(model_t *mod, entity_state_t* entstate)
{
	if (mod->type != mod_studio)
		return 0;

	auto studiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(mod);

	if (!studiohdr)
		return 0;

	int sequence = entstate->sequence;
	if (sequence >= studiohdr->numseq)
		return 0;

	auto pseqdesc = (mstudioseqdesc_t*)((byte*)studiohdr + studiohdr->seqindex) + sequence;

	if (
		pseqdesc->activity == ACT_DIESIMPLE ||
		pseqdesc->activity == ACT_DIEBACKWARD ||
		pseqdesc->activity == ACT_DIEFORWARD ||
		pseqdesc->activity == ACT_DIEVIOLENT ||
		pseqdesc->activity == ACT_DIEVIOLENT ||
		pseqdesc->activity == ACT_DIE_HEADSHOT ||
		pseqdesc->activity == ACT_DIE_CHESTSHOT ||
		pseqdesc->activity == ACT_DIE_GUTSHOT ||
		pseqdesc->activity == ACT_DIE_BACKSHOT
		)
	{
		return 1;
	}

	if (
		pseqdesc->activity == ACT_BARNACLE_HIT ||
		pseqdesc->activity == ACT_BARNACLE_PULL ||
		pseqdesc->activity == ACT_BARNACLE_CHOMP ||
		pseqdesc->activity == ACT_BARNACLE_CHEW
		)
	{
		return 2;
	}

	return 0;
}

std::shared_ptr<IShadowTexture> R_CreateSingleShadowTexture(uint32_t size, bool bStatic)
{
	return std::make_shared<CSingleShadowTexture>(size, bStatic);
}

std::shared_ptr<IShadowTexture> R_CreateCascadedShadowTexture(uint32_t size, bool bStatic)
{
	return std::make_shared<CCascadedShadowTexture>(size, bStatic);
}

std::shared_ptr<IShadowTexture> R_CreateCubemapShadowTexture(uint32_t size, bool bStatic)
{
	return std::make_shared<CCubemapShadowTexture>(size, bStatic);
}

void R_InitShadow(void)
{
	r_shadow = gEngfuncs.pfnRegisterVariable("r_shadow", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
}

void R_ShutdownShadow(void)
{
	g_pCurrentShadowTexture = nullptr;

	for (int i = 0; i < _countof(g_DLightShadowTextures); ++i)
	{
		g_DLightShadowTextures[i].reset();
	}
}

bool R_ShouldRenderShadow(void)
{
	if (R_IsRenderingShadowView())
		return false;

	if (R_IsRenderingWaterView())
		return false;

	if (R_IsRenderingPortal())
		return false;

	if (gPrivateFuncs.CL_IsDevOverviewMode())
		return false;

	return r_shadow->value ? true : false;
}

bool R_ShouldCastShadow(cl_entity_t *ent)
{
	if(!ent)
		return false;

	if(!ent->model)
		return false;

	if (ent->curstate.rendermode != kRenderNormal)
		return false;

	if (ent->model->type == mod_studio)
	{
		if (ent->curstate.effects & EF_NODRAW)
			return false;

		//player model always render shadow
		if (!strcmp(ent->model->name, "models/player.mdl"))
			return true;

		if (ent->player)
			return true;

		//BulletPhysics ragdoll corpse
		if (ent->curstate.iuser4 == PhyCorpseFlag)
			return true;

		if (ent->index == 0)
			return false;

		if (ent->curstate.movetype == MOVETYPE_NONE && ent->curstate.solid == SOLID_NOT)
			return false;

		if (g_iEngineType == ENGINE_SVENGINE)
		{
			if (ent->curstate.effects & EF_NOSHADOW)
				return false;
		}

		return true;
	}

	return false;
}

void R_SetupShadowMatrix(float out[4][4], const float worldMatrix[4][4], const float projMatrix[4][4])
{
	/*
	Counterpart of following matrix:
		const float bias[16] = {
				0.5f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.5f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.5f, 0.0f,
				0.5f, 0.5f, 0.5f, 1.0f
			};
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glLoadIdentity();
		glLoadMatrixf(bias);
		glMultMatrixf(offsetMatrix); // CSM offset matrix
		glMultMatrixf(r_projection_matrix);
		glMultMatrixf(r_world_matrix);
		glGetFloatv(GL_TEXTURE_MATRIX, (float *)shadowmatrix);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	*/

	const float bias[16] = {
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f
	};

	// First multiply projection matrix with world matrix
	float projWorldMatrix[4][4];
	Matrix4x4_Multiply(projWorldMatrix, worldMatrix, projMatrix);

	// Then multiply bias matrix with the result
	Matrix4x4_Multiply(out, projWorldMatrix, (const float (*)[4])bias);
}

void R_RenderShadowmapForDynamicLights(void)
{
	if (!r_deferred_lighting->value)
		return;

	if (R_ShouldRenderShadow())
	{
		GL_BeginDebugGroup("R_RenderShadowmapForDynamicLights");

		const auto PointLightCallback = [](PointLightCallbackArgs *args, void *context)
		{
			if (args->ppShadowTexture)
			{
				if ((*args->ppShadowTexture) == nullptr || (*args->ppShadowTexture)->IsCubemap() != true || (*args->ppShadowTexture)->IsStatic() != false || (*args->ppShadowTexture)->GetTextureSize() != 1024)
				{
					(*args->ppShadowTexture) = R_CreateCubemapShadowTexture(1024, false);
				}

				if ((*args->ppShadowTexture) && !(*args->ppShadowTexture)->IsReady())
				{
					r_draw_shadowview = true;
					r_draw_multiview = true;
					r_draw_nofrustumcull = true;

					g_pCurrentShadowTexture = (*args->ppShadowTexture);

					g_pCurrentShadowTexture->SetViewport(0, 0, g_pCurrentShadowTexture->GetTextureSize(), g_pCurrentShadowTexture->GetTextureSize());

					GL_BeginDebugGroup("R_RenderShadowDynamicLights - DrawPointlightShadowPass");

					GL_BindFrameBufferWithTextures(&s_ShadowFBO, 0, 0, g_pCurrentShadowTexture->GetDepthTexture(), g_pCurrentShadowTexture->GetTextureSize(), g_pCurrentShadowTexture->GetTextureSize());
					glDrawBuffer(GL_NONE);
					glReadBuffer(GL_NONE);

					// For cubemap shadow with reversed-Z, clear depth to 0.0 (far plane)
					GL_ClearDepthStencil(0.0f, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

					glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
					// For reversed-Z cubemap shadow: use GEQUAL and reversed depth range
					// This maps near plane (NDC=1) to depth=1.0, far plane (NDC=-1) to depth=0.0
					glDepthFunc(GL_GEQUAL);
					glDepthRange(1.0, 0.0);  // Reversed: near=1.0, far=0.0

					R_PushRefDef();

					r_viewport[0] = g_pCurrentShadowTexture->GetViewport()[0];
					r_viewport[1] = g_pCurrentShadowTexture->GetViewport()[1];
					r_viewport[2] = g_pCurrentShadowTexture->GetViewport()[2];
					r_viewport[3] = g_pCurrentShadowTexture->GetViewport()[3];

					glViewport(r_viewport[0], r_viewport[1], r_viewport[2], r_viewport[3]);

					// Calculate 6 faces for cubemap shadow mapping
					// OpenGL cubemap face order: +X, -X, +Y, -Y, +Z, -Z
					const vec3_t cubemapAngles[] = {
						{0, 0, 90},     
						{0, 180, 270},  
						{0, 90, 0},     
						{0, 270, 180},  
						{-90, 270, 0},  
						{90, 270, 0},   
					};

					camera_ubo_t CameraUBO{};

					CameraUBO.numViews = 6;

					for (int i = 0; i < 6; ++i)
					{
						VectorCopy(args->origin, (*r_refdef.vieworg));
						VectorCopy(cubemapAngles[i], (*r_refdef.viewangles));
						R_UpdateRefDef();

						R_LoadIdentityForProjectionMatrix();
						// Use reversed-Z projection for better depth precision in cubemap shadow
						R_SetupPerspectiveReversedZ(90, 90, 0.1f, args->radius);

						R_LoadIdentityForWorldMatrix();
						R_SetupPlayerViewWorldMatrix((*r_refdef.vieworg), (*r_refdef.viewangles));

						R_SetFrustum(90, 90, r_frustum_right, r_frustum_top);

						auto worldMatrix = (float (*)[4][4])R_GetWorldMatrix();
						auto projMatrix = (float (*)[4][4])R_GetProjectionMatrix();

						mat4 shadowMatrix;
						R_SetupShadowMatrix(shadowMatrix, (*worldMatrix), (*projMatrix));

						g_pCurrentShadowTexture->SetWorldMatrix(i, worldMatrix);
						g_pCurrentShadowTexture->SetProjectionMatrix(i, projMatrix);
						g_pCurrentShadowTexture->SetShadowMatrix(i, &shadowMatrix);

						R_SetupCameraView(&CameraUBO.views[i]);
					}

					GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, 0, sizeof(CameraUBO), &CameraUBO);

					{
						auto old_draw_classify = r_draw_classify;
						r_draw_classify &= ~DRAW_CLASSIFY_TRANS_ENTITIES;
						r_draw_classify &= ~DRAW_CLASSIFY_PARTICLES;
						r_draw_classify &= ~DRAW_CLASSIFY_WATER;
						r_draw_classify &= ~DRAW_CLASSIFY_DECAL;

						R_RenderScene();

						r_draw_classify = old_draw_classify;
					}

					R_PopRefDef();

					glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
					// Restore normal depth function and range
					glDepthFunc(GL_LEQUAL);
					glDepthRange(0.0, 1.0);  // Restore standard depth range

					r_draw_shadowview = false;
					r_draw_multiview = false;
					r_draw_nofrustumcull = false;

					GL_EndDebugGroup();

					g_pCurrentShadowTexture->SetReady(true);

					g_pCurrentShadowTexture = nullptr;
				}
			}
		};

		const auto SpotLightCallback = [](SpotLightCallbackArgs *args, void *context)
		{
			if (args->ppShadowTexture && args->bIsFromLocalPlayer)
			{
				if ((*args->ppShadowTexture) == nullptr || (*args->ppShadowTexture)->IsSingleLayer() != true || (*args->ppShadowTexture)->IsStatic() != false || (*args->ppShadowTexture)->GetTextureSize() != 1024)
				{
					(*args->ppShadowTexture) = R_CreateSingleShadowTexture(1024, false);
				}

				if ((*args->ppShadowTexture) && !(*args->ppShadowTexture)->IsReady())
				{
					r_draw_shadowview = true;
					r_draw_multiview = true;

					g_pCurrentShadowTexture = (*args->ppShadowTexture);

					g_pCurrentShadowTexture->SetViewport(0, 0, g_pCurrentShadowTexture->GetTextureSize(), g_pCurrentShadowTexture->GetTextureSize());

					GL_BeginDebugGroup("R_RenderShadowDynamicLights - DrawSpotlightShadowPass");

					GL_BindFrameBufferWithTextures(&s_ShadowFBO, 0, 0, g_pCurrentShadowTexture->GetDepthTexture(), g_pCurrentShadowTexture->GetTextureSize(), g_pCurrentShadowTexture->GetTextureSize());
					glDrawBuffer(GL_NONE);
					glReadBuffer(GL_NONE);

					GL_ClearDepthStencil(1.0f, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

					glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

					R_PushRefDef();

					VectorCopy(args->origin, (*r_refdef.vieworg));
					VectorCopy(args->angle, (*r_refdef.viewangles));
					R_UpdateRefDef();

					r_viewport[0] = g_pCurrentShadowTexture->GetViewport()[0];
					r_viewport[1] = g_pCurrentShadowTexture->GetViewport()[1];
					r_viewport[2] = g_pCurrentShadowTexture->GetViewport()[2];
					r_viewport[3] = g_pCurrentShadowTexture->GetViewport()[3];

					glViewport(r_viewport[0], r_viewport[1], r_viewport[2], r_viewport[3]);

					R_LoadIdentityForWorldMatrix();
					R_SetupPlayerViewWorldMatrix((*r_refdef.vieworg), (*r_refdef.viewangles));

					float cone_fov = args->coneAngle * 2 * 360 / (M_PI * 2);

					R_LoadIdentityForProjectionMatrix();
					R_SetupPerspective(cone_fov, cone_fov, gl_nearplane->value, args->distance);

					R_SetFrustum(r_xfov_currentpass, r_yfov_currentpass, r_frustum_right, r_frustum_top);

					auto worldMatrix = (float (*)[4][4])R_GetWorldMatrix();
					auto projMatrix = (float (*)[4][4])R_GetProjectionMatrix();

					mat4 shadowMatrix;
					R_SetupShadowMatrix(shadowMatrix, (*worldMatrix), (*projMatrix));

					g_pCurrentShadowTexture->SetWorldMatrix(0, worldMatrix);
					g_pCurrentShadowTexture->SetProjectionMatrix(0, projMatrix);
					g_pCurrentShadowTexture->SetShadowMatrix(0, &shadowMatrix);

					camera_ubo_t CameraUBO;
					R_SetupCameraView(&CameraUBO.views[0]);
					CameraUBO.numViews = 1;
					GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, 0, sizeof(CameraUBO), &CameraUBO);

					auto pLocalPlayer = gEngfuncs.GetLocalPlayer();

					if (pLocalPlayer->model && args->bIsFromLocalPlayer)
					{
						auto old_draw_classify = r_draw_classify;
						r_draw_classify &= ~DRAW_CLASSIFY_LOCAL_PLAYER;
						r_draw_classify &= ~DRAW_CLASSIFY_TRANS_ENTITIES;
						r_draw_classify &= ~DRAW_CLASSIFY_PARTICLES;
						r_draw_classify &= ~DRAW_CLASSIFY_DECAL;
						r_draw_classify &= ~DRAW_CLASSIFY_WATER;

						R_RenderScene();

						r_draw_classify = old_draw_classify;
					}
					else
					{
						auto old_draw_classify = r_draw_classify;
						r_draw_classify &= ~DRAW_CLASSIFY_TRANS_ENTITIES;
						r_draw_classify &= ~DRAW_CLASSIFY_PARTICLES;
						r_draw_classify &= ~DRAW_CLASSIFY_DECAL;
						r_draw_classify &= ~DRAW_CLASSIFY_WATER;

						R_RenderScene();

						r_draw_classify = old_draw_classify;
					}

					R_PopRefDef();

					glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

					r_draw_multiview = false;
					r_draw_shadowview = false;

					GL_EndDebugGroup();

					g_pCurrentShadowTexture->SetReady(true);

					g_pCurrentShadowTexture = nullptr;
				}
			}
		};

		const auto DirectionalLightCallback = [](DirectionalLightCallbackArgs* args, void* context)
		{
			if (args->ppShadowTexture)
			{
				if ((*args->ppShadowTexture) == nullptr || (*args->ppShadowTexture)->IsSingleLayer() != true || (*args->ppShadowTexture)->IsStatic() != true || (*args->ppShadowTexture)->GetTextureSize() != 8192)
				{
					(*args->ppShadowTexture) = R_CreateSingleShadowTexture(8192, true);
				}

				if ((*args->ppShadowTexture) && !(*args->ppShadowTexture)->IsReady())
				{
					auto pWorldSurfaceModel = R_GetWorldSurfaceModel(*(cl_worldmodel));

					r_draw_shadowview = true;
					r_draw_multiview = true;
					r_draw_nofrustumcull = true;

					g_pCurrentShadowTexture = (*args->ppShadowTexture);

					g_pCurrentShadowTexture->SetViewport(0, 0, g_pCurrentShadowTexture->GetTextureSize(), g_pCurrentShadowTexture->GetTextureSize());

					GL_BeginDebugGroup("R_RenderShadowDynamicLights - DrawDirectionalLightStatic");

					GL_BindFrameBufferWithTextures(&s_ShadowFBO, 0, 0, g_pCurrentShadowTexture->GetDepthTexture(), g_pCurrentShadowTexture->GetTextureSize(), g_pCurrentShadowTexture->GetTextureSize());
					glDrawBuffer(GL_NONE);
					glReadBuffer(GL_NONE);

					GL_ClearDepthStencil(1.0f, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

					glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

					R_PushRefDef();

					VectorCopy(args->origin, (*r_refdef.vieworg));
					VectorCopy(args->angle, (*r_refdef.viewangles));
					R_UpdateRefDef();

					r_viewport[0] = g_pCurrentShadowTexture->GetViewport()[0];
					r_viewport[1] = g_pCurrentShadowTexture->GetViewport()[1];
					r_viewport[2] = g_pCurrentShadowTexture->GetViewport()[2];
					r_viewport[3] = g_pCurrentShadowTexture->GetViewport()[3];

					glViewport(r_viewport[0], r_viewport[1], r_viewport[2], r_viewport[3]);

					R_LoadIdentityForWorldMatrix();
					R_SetupPlayerViewWorldMatrix((*r_refdef.vieworg), (*r_refdef.viewangles));

					// Set up orthographic projection for this cascade
					float orthoSize = args->size; // Increase size for further cascades

					R_LoadIdentityForProjectionMatrix();
					R_SetupOrthoProjectionMatrix(-orthoSize / 2, orthoSize / 2, -orthoSize / 2, orthoSize / 2, 2048, -2048, true);

					auto worldMatrix = (float (*)[4][4])R_GetWorldMatrix();
					auto projMatrix = (float (*)[4][4])R_GetProjectionMatrix();

					mat4 shadowMatrix;
					R_SetupShadowMatrix(shadowMatrix, (*worldMatrix), (*projMatrix));

					g_pCurrentShadowTexture->SetWorldMatrix(0, worldMatrix);
					g_pCurrentShadowTexture->SetProjectionMatrix(0, projMatrix);
					g_pCurrentShadowTexture->SetShadowMatrix(0, &shadowMatrix);

					camera_ubo_t CameraUBO;
					R_SetupCameraView(&CameraUBO.views[0]);
					CameraUBO.numViews = 1;
					GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, 0, sizeof(CameraUBO), &CameraUBO);

					auto old_brush_polys = (*c_brush_polys);
					(*c_brush_polys) = 0;

					auto old_draw_classify = r_draw_classify;
					r_draw_classify = DRAW_CLASSIFY_WORLD;
					
					R_RenderScene();

					bool bAnyPolyRendered = (*c_brush_polys) > 0 ? true : false;

					r_draw_classify = old_draw_classify;
					(*c_brush_polys) = old_brush_polys;

					R_PopRefDef();

					glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

					r_draw_shadowview = false;
					r_draw_multiview = false;
					r_draw_nofrustumcull = false;

					GL_EndDebugGroup();

					g_pCurrentShadowTexture->SetReady(bAnyPolyRendered);

					g_pCurrentShadowTexture = nullptr;
				}
			}

			if (args->ppCSMShadowTexture)
			{
				// Allocate 4096x4096 CSM texture if not already allocated
				if ((*args->ppCSMShadowTexture) == nullptr || (*args->ppCSMShadowTexture)->IsCascaded() != true || (*args->ppCSMShadowTexture)->IsStatic() != false || (*args->ppCSMShadowTexture)->GetTextureSize() != CSM_RESOLUTION)
				{
					(*args->ppCSMShadowTexture) = R_CreateCascadedShadowTexture(CSM_RESOLUTION, false);
				}

				if ((*args->ppCSMShadowTexture) && !(*args->ppCSMShadowTexture)->IsReady())
				{
					g_pCurrentShadowTexture = (*args->ppCSMShadowTexture);

					r_draw_shadowview = true;
					r_draw_multiview = true;
					r_draw_nofrustumcull = true;

					const float lambda = 0.5f; // 例如0.8，也可来自cvar
					const float orthoMargin = 1.15f; // 15% 外扩，避免裁边

					// Calculate cascade distances based on camera frustum
					// These could be configurable via cvars in the future
					float nearPlane = r_znear;  // Should match r_nearclip or similar, 4.0 by default
					float farPlane = r_zfar; // Should match r_farclip or similar, 8192.0 by default

					float fovY = (r_yfov_currentpass) * (M_PI / 360.0);
					float fovX = (r_xfov_currentpass) * (M_PI / 360.0);
					float tanHalfFovY = tanf(0.5f * fovY);
					float tanHalfFovX = tanf(0.5f * fovX);

					float splits[CSM_LEVELS + 1]{};
					splits[0] = nearPlane;

					// Use logarithmic distribution for cascades
					for (int i = 1; i <= CSM_LEVELS; ++i)
					{
						float si = (float)i / (float)CSM_LEVELS; // [0,1]
						float d_lin = nearPlane + (farPlane - nearPlane) * si;
						float d_log = nearPlane * powf(farPlane / nearPlane, si);
						splits[i] = d_lin * (1.0f - lambda) + d_log * lambda;
					}

					for (int i = 0; i < CSM_LEVELS; ++i)
					{
						float csmFar = splits[i + 1];
						g_pCurrentShadowTexture->SetCSMDistance(i, csmFar);
					}

					g_pCurrentShadowTexture->SetViewport(0, 0, CSM_RESOLUTION, CSM_RESOLUTION);

					GL_BeginDebugGroup("R_RenderShadowDynamicLights - DrawDirectionalLightCSM");

					GL_BindFrameBuffer(&s_ShadowFBO);

					// Bind texture array layers to framebuffer - we'll use geometry shader to select layer
					// Note: We can't use glFramebufferTexture because that requires all layers, 
					// but clearing needs to be done per-layer in a loop
					for (int i = 0; i < CSM_LEVELS; ++i)
					{
						glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, g_pCurrentShadowTexture->GetDepthTexture(), 0, i);
						GL_ClearDepthStencil(1.0f, STENCIL_MASK_NONE, STENCIL_MASK_ALL);
					}

					// Now bind all layers for rendering
					glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, g_pCurrentShadowTexture->GetDepthTexture(), 0);

					glDrawBuffer(GL_NONE);
					glReadBuffer(GL_NONE);

					glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

					R_PushRefDef();

					// All cascades use same viewangles and vieworg
					VectorCopy(args->angle, (*r_refdef.viewangles));
					R_UpdateRefDef();

					// All cascades use same worldmatrix
					R_LoadIdentityForWorldMatrix();
					R_SetupPlayerViewWorldMatrix((*r_refdef.vieworg), (*r_refdef.viewangles));

					r_viewport[0] = g_pCurrentShadowTexture->GetViewport()[0];
					r_viewport[1] = g_pCurrentShadowTexture->GetViewport()[1];
					r_viewport[2] = g_pCurrentShadowTexture->GetViewport()[2];
					r_viewport[3] = g_pCurrentShadowTexture->GetViewport()[3];

					glViewport(r_viewport[0], r_viewport[1], r_viewport[2], r_viewport[3]);

					// Setup camera UBO with all cascade views
					camera_ubo_t CameraUBO;
					CameraUBO.numViews = CSM_LEVELS;

					// Calculate projection matrices for all cascades and setup shadow matrices
					for (int cascadeIndex = 0; cascadeIndex < CSM_LEVELS; ++cascadeIndex)
					{
						float splitNear = splits[cascadeIndex + 0];
						float splitFar = splits[cascadeIndex + 1];

						// 该级联在相机视锥上界面的半宽/半高（取far端，因为更大）
						float halfW_far = splitFar * tanHalfFovX;
						float halfH_far = splitFar * tanHalfFovY;

						// 该级联厚度的一半
						float halfDepth = 0.5f * (splitFar - splitNear);

						// 用包含该截头棱锥的最小球近似，半径为到far平面角点的最大距离
						// 与光方向无关，稳定且不会裁边
						float radius = sqrtf(halfW_far * halfW_far + halfH_far * halfH_far + halfDepth * halfDepth);

						// 正交投影尺寸（正方形），加一点margin避免抖动时裁边
						float orthoSize = 2.0f * radius * orthoMargin;

						R_LoadIdentityForProjectionMatrix();
						R_SetupOrthoProjectionMatrix(-orthoSize / 2, orthoSize / 2, -orthoSize / 2, orthoSize / 2, 2048, -2048, true);

						auto worldMatrix = (float (*)[4][4])R_GetWorldMatrix();
						auto projMatrix = (float (*)[4][4])R_GetProjectionMatrix();

						mat4 shadowMatrix;
						R_SetupShadowMatrix(shadowMatrix, (*worldMatrix), (*projMatrix));

						g_pCurrentShadowTexture->SetWorldMatrix(cascadeIndex, worldMatrix);
						g_pCurrentShadowTexture->SetProjectionMatrix(cascadeIndex, projMatrix);
						g_pCurrentShadowTexture->SetShadowMatrix(cascadeIndex, &shadowMatrix);

						// Setup camera view for this cascade in the UBO
						R_SetupCameraView(&CameraUBO.views[cascadeIndex]);
					}

					// Upload all views to UBO
					GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, 0, sizeof(CameraUBO), &CameraUBO);

					auto old_draw_classify = r_draw_classify;
					r_draw_classify = (DRAW_CLASSIFY_OPAQUE_ENTITIES | DRAW_CLASSIFY_LOCAL_PLAYER);

					// Render all cascades in a single draw call using multiview geometry shader
					R_RenderScene();

					r_draw_classify = old_draw_classify;

					R_PopRefDef();

					glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

					r_draw_shadowview = false;
					r_draw_multiview = false;
					r_draw_nofrustumcull = false;

					GL_EndDebugGroup();

					g_pCurrentShadowTexture->SetReady(true);

					g_pCurrentShadowTexture = nullptr;
				}
			}
		};

		R_IterateDynamicLights(PointLightCallback, SpotLightCallback, DirectionalLightCallback, nullptr);

		GL_EndDebugGroup();
	}
}

/*

	Purpose : Clear shadow related vars which might be accessed later by deferred lighting pass.

*/

void R_RenderShadowMap_Start(void)
{
	for (int i = 0; i < _countof(g_DLightShadowTextures); ++i)
	{
		if (g_DLightShadowTextures[i] && !g_DLightShadowTextures[i]->IsStatic())
		{
			g_DLightShadowTextures[i]->SetReady(false);
		}
	}

	for (size_t i = 0; i < g_DynamicLights.size(); ++i)
	{
		const auto& pDynamicLight = g_DynamicLights[i];

		if (pDynamicLight->pShadowTexture && !pDynamicLight->pShadowTexture->IsStatic())
		{
			pDynamicLight->pShadowTexture->SetReady(false);
		}

		if (pDynamicLight->pCSMShadowTexture && !pDynamicLight->pCSMShadowTexture->IsStatic())
		{
			pDynamicLight->pCSMShadowTexture->SetReady(false);
		}
	}
}

/*

	Purpose : Rendering textures for shadow mapping

*/

void R_RenderShadowMap(void)
{
	R_RenderShadowMap_Start();

	if (!r_shadow->value)
		return;

	R_RenderShadowmapForDynamicLights();
}