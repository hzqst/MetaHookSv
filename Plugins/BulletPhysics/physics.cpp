#include "exportfuncs.h"
#include <triangleapi.h>
#include <studio.h>
#include <cvardef.h>
#include "enginedef.h"
#include "plugins.h"
#include "privatehook.h"
#include "physics.h"
#include "qgl.h"
#include "mathlib.h"

btScalar G2BScale = 1;
btScalar B2GScale = 1 / G2BScale;

extern studiohdr_t **pstudiohdr;
extern model_t **r_model;
extern float(*pbonetransform)[128][3][4];
extern float(*plighttransform)[128][3][4]; 
extern cvar_t *bv_debug;
extern cvar_t *bv_simrate;
extern cvar_t *bv_scale;
extern model_t *r_worldmodel;
extern cl_entity_t *r_worldentity;
extern int *r_visframecount;

int EngineGetNumKnownModel(void);
int EngineGetMaxKnownModel(void);
int EngineGetModelIndex(model_t *mod);
model_t *EngineGetModelByIndex(int index);
bool IsEntityPresent(cl_entity_t* ent);
bool IsEntityGargantua(cl_entity_t* ent);
bool IsEntityBarnacle(cl_entity_t* ent);
bool IsEntityWater(cl_entity_t* ent);
bool IsEntityEmitted(cl_entity_t* ent);
int StudioGetSequenceActivityType(model_t *mod, entity_state_t* entstate);
void RagdollDestroyCallback(int entindex);

const float r_identity_matrix[4][4] = {
	{1.0f, 0.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 0.0f, 1.0f}
};

void Matrix3x4ToTransform(const float matrix3x4[3][4], btTransform &trans)
{
	float matrix4x4[4][4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f}
	};
	memcpy(matrix4x4, matrix3x4, sizeof(float[3][4]));

	float matrix4x4_transposed[4][4];
	Matrix4x4_Transpose(matrix4x4_transposed, matrix4x4);

	trans.setFromOpenGLMatrix((float *)matrix4x4_transposed);
}

void TransformToMatrix3x4(const btTransform &trans, float matrix3x4[3][4])
{
	float matrix4x4_transposed[4][4];
	trans.getOpenGLMatrix((float *)matrix4x4_transposed);

	float matrix4x4[4][4];
	Matrix4x4_Transpose(matrix4x4, matrix4x4_transposed);

	memcpy(matrix3x4, matrix4x4, sizeof(float[3][4]));
}

//GoldSrcToBullet Scaling

void FloatGoldSrcToBullet(float *trans)
{
	(*trans) *= G2BScale;
}

void TransformGoldSrcToBullet(btTransform &trans)
{
	auto &org = trans.getOrigin();

	org.m_floats[0] *= G2BScale;
	org.m_floats[1] *= G2BScale;
	org.m_floats[2] *= G2BScale;
}

void Vec3GoldSrcToBullet(vec3_t vec)
{
	vec[0] *= G2BScale;
	vec[1] *= G2BScale;
	vec[2] *= G2BScale;
}

void Vector3GoldSrcToBullet(btVector3& vec)
{
	vec.m_floats[0] *= G2BScale;
	vec.m_floats[1] *= G2BScale;
	vec.m_floats[2] *= G2BScale;
}

//BulletToGoldSrc Scaling

void TransformBulletToGoldSrc(btTransform &trans)
{
	trans.getOrigin().m_floats[0] *= B2GScale;
	trans.getOrigin().m_floats[1] *= B2GScale;
	trans.getOrigin().m_floats[2] *= B2GScale;
}

void Vec3BulletToGoldSrc(vec3_t vec)
{
	vec[0] *= B2GScale;
	vec[1] *= B2GScale;
	vec[2] *= B2GScale;
}

void Vector3BulletToGoldSrc(btVector3& vec)
{
	vec.m_floats[0] *= B2GScale;
	vec.m_floats[1] *= B2GScale;
	vec.m_floats[2] *= B2GScale;
}

void CPhysicsDebugDraw::drawLine(const btVector3& from1, const btVector3& to1, const btVector3& color1)
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glLineWidth(1);

	gEngfuncs.pTriAPI->Color4f(color1.getX(), color1.getY(), color1.getZ(), 1.0f);
	gEngfuncs.pTriAPI->Begin(TRI_LINES);

	vec3_t from = { from1.getX(), from1.getY(), from1.getZ() };
	vec3_t to = { to1.getX(), to1.getY(), to1.getZ() };

	Vec3BulletToGoldSrc(from);
	Vec3BulletToGoldSrc(to);

	gEngfuncs.pTriAPI->Vertex3fv(from);
	gEngfuncs.pTriAPI->Vertex3fv(to);
	gEngfuncs.pTriAPI->End();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
}

CPhysicsManager gPhysicsManager;

CPhysicsManager::CPhysicsManager()
{
	 m_collisionConfiguration = NULL;
	 m_dispatcher = NULL;
	 m_overlappingPairCache = NULL;
	 m_solver = NULL;
	 m_dynamicsWorld = NULL;
	 m_debugDraw = NULL;
	 m_worldVertexArray = NULL;
	 m_barnacleIndexArray = NULL;
	 m_barnacleVertexArray = NULL;
	 m_gargantuaIndexArray = NULL;
	 m_gargantuaVertexArray = NULL;
	 m_gravity = 0;
}

void CPhysicsManager::GenerateBrushIndiceArray(void)
{
	int maxNum = EngineGetMaxKnownModel();

	if ((int)m_brushIndexArray.size() < maxNum)
		m_brushIndexArray.resize(maxNum);

	for (int i = 0; i < maxNum; ++i)
	{
		if (m_brushIndexArray[i])
		{
			delete m_brushIndexArray[i];
			m_brushIndexArray[i] = NULL;
		}
	}

	for (int i = 0; i < EngineGetNumKnownModel(); ++i)
	{
		auto mod = EngineGetModelByIndex(i);
		if (mod->type == mod_brush && mod->name[0])
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			{
				m_brushIndexArray[i] = new indexarray_t;
				GenerateIndexedArrayForBrush(mod, m_worldVertexArray, m_brushIndexArray[i]);
			}
		}
	}
}

void CPhysicsManager::GenerateWorldVerticeArray(void)
{
	if (m_worldVertexArray) {
		delete m_worldVertexArray;
		m_worldVertexArray = NULL;
	}

	m_worldVertexArray = new vertexarray_t;

	brushvertex_t Vertexes[3];

	int iNumFaces = 0;
	int iNumVerts = 0;

	auto surf = r_worldmodel->surfaces;

	m_worldVertexArray->vFaceBuffer.resize(r_worldmodel->numsurfaces);

	for (int i = 0; i < r_worldmodel->numsurfaces; i++)
	{
		if ((surf[i].flags & (SURF_DRAWTURB | SURF_UNDERWATER | SURF_DRAWSKY)))
			continue;

		auto poly = surf[i].polys;

		poly->flags = i;

		brushface_t *brushface = &m_worldVertexArray->vFaceBuffer[i];

		int iStartVert = iNumVerts;

		brushface->start_vertex = iStartVert;

		for (poly = surf[i].polys; poly; poly = poly->next)
		{
			auto v = poly->verts[0];

			for (int j = 0; j < 3; j++, v += VERTEXSIZE)
			{
				Vertexes[j].pos[0] = v[0];
				Vertexes[j].pos[1] = v[1];
				Vertexes[j].pos[2] = v[2];
				Vec3GoldSrcToBullet(Vertexes[j].pos);
			}
			m_worldVertexArray->vVertexBuffer.emplace_back(Vertexes[0]);
			m_worldVertexArray->vVertexBuffer.emplace_back(Vertexes[1]);
			m_worldVertexArray->vVertexBuffer.emplace_back(Vertexes[2]);
			iNumVerts += 3;

			for (int j = 0; j < (poly->numverts - 3); j++, v += VERTEXSIZE)
			{
				memcpy(&Vertexes[1], &Vertexes[2], sizeof(brushvertex_t));

				Vertexes[2].pos[0] = v[0];
				Vertexes[2].pos[1] = v[1];
				Vertexes[2].pos[2] = v[2];
				Vec3GoldSrcToBullet(Vertexes[2].pos);

				m_worldVertexArray->vVertexBuffer.emplace_back(Vertexes[0]);
				m_worldVertexArray->vVertexBuffer.emplace_back(Vertexes[1]);
				m_worldVertexArray->vVertexBuffer.emplace_back(Vertexes[2]);
				iNumVerts += 3;
			}
		}

		brushface->num_vertexes = iNumVerts - iStartVert;
	}
}

void CPhysicsManager::GenerateIndexedArrayForBrushface(brushface_t *brushface, indexarray_t *indexarray)
{
	int first = -1;
	int prv0 = -1;
	int prv1 = -1;
	int prv2 = -1;
	for (int i = 0; i < brushface->num_vertexes; i++)
	{
		if (prv0 != -1 && prv1 != -1 && prv2 != -1)
		{
			indexarray->vIndiceBuffer.emplace_back(brushface->start_vertex + first);
			indexarray->vIndiceBuffer.emplace_back(brushface->start_vertex + prv2);
		}

		indexarray->vIndiceBuffer.emplace_back(brushface->start_vertex + i);

		if (first == -1)
			first = i;

		prv0 = prv1;
		prv1 = prv2;
		prv2 = i;
	}
}

void CPhysicsManager::GenerateIndexedArrayForSurface(msurface_t *psurf, vertexarray_t *vertexarray, indexarray_t *indexarray)
{
	if (psurf->flags & SURF_DRAWTURB)
	{
		return;
	}

	if (psurf->flags & SURF_DRAWSKY)
	{
		return;
	}

	if (psurf->flags & SURF_UNDERWATER)
	{
		return;
	}

	GenerateIndexedArrayForBrushface(&vertexarray->vFaceBuffer[psurf->polys->flags], indexarray);	
}

void CPhysicsManager::GenerateIndexedArrayRecursiveWorldNode(mnode_t *node, vertexarray_t *vertexarray, indexarray_t *indexarray)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->contents < 0)
		return;

	GenerateIndexedArrayRecursiveWorldNode(node->children[0], vertexarray, indexarray);

	auto c = node->numsurfaces;

	if (c)
	{
		auto psurf = r_worldmodel->surfaces + node->firstsurface;

		for (; c; c--, psurf++)
		{
			GenerateIndexedArrayForSurface(psurf, vertexarray, indexarray);
		}
	}

	GenerateIndexedArrayRecursiveWorldNode(node->children[1], vertexarray, indexarray);
}

void CPhysicsManager::GenerateIndexedArrayForBrush(model_t *mod, vertexarray_t *vertexarray, indexarray_t *indexarray)
{
	if (mod == r_worldmodel)
	{
		GenerateIndexedArrayRecursiveWorldNode(mod->nodes, vertexarray, indexarray);
	}
	else
	{
		auto psurf = &mod->surfaces[mod->firstmodelsurface];
		for (int i = 0; i < mod->nummodelsurfaces; i++, psurf++)
		{
			GenerateIndexedArrayForSurface(psurf, vertexarray, indexarray);			
		}
	}
}

CStaticBody *CPhysicsManager::CreateStaticBody(cl_entity_t *ent, vertexarray_t *vertexarray, indexarray_t *indexarray, bool kinematic)
{
	if (!indexarray->vIndiceBuffer.size())
	{
		auto staticbody = new CStaticBody;

		staticbody->m_rigbody = NULL;
		staticbody->m_entindex = ent->index;
		staticbody->m_vertexarray = vertexarray;
		staticbody->m_indexarray = indexarray;

		m_staticMap[ent->index] = staticbody;
		return staticbody;
	}

	auto staticbody = new CStaticBody;
	staticbody->m_entindex = ent->index;
	staticbody->m_vertexarray = vertexarray;
	staticbody->m_indexarray = indexarray;

	auto vertexArray = new btTriangleIndexVertexArray(
		indexarray->vIndiceBuffer.size() / 3, indexarray->vIndiceBuffer.data(), 3 * sizeof(int),
		vertexarray->vVertexBuffer.size(), (float *)vertexarray->vVertexBuffer.data(), sizeof(brushvertex_t));

	btRigidBody::btRigidBodyConstructionInfo cInfo(0, new EntityMotionState(ent), new btBvhTriangleMeshShape(vertexArray, true, true));
	cInfo.m_friction = 1;
	cInfo.m_rollingFriction = 1;

	btRigidBody* body = new btRigidBody(cInfo);

	staticbody->m_rigbody = body;

	m_dynamicsWorld->addRigidBody(body, staticbody->m_group, staticbody->m_mask);

	if (kinematic)
	{
		body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		body->setActivationState(DISABLE_DEACTIVATION);

		staticbody->m_kinematic = true;
	}
	else
	{
		body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
	}

	m_staticMap[ent->index] = staticbody;

	return staticbody;
}

void CPhysicsManager::RotateForEntity(cl_entity_t *e, float matrix[4][4])
{
	int i;
	vec3_t angles;
	vec3_t modelpos;

	VectorCopy(e->origin, modelpos);
	VectorCopy(e->angles, angles);

	if (e->curstate.movetype != MOVETYPE_NONE)
	{
		float f;
		float d;

		if (e->curstate.animtime + 0.2f > gEngfuncs.GetClientTime() && e->curstate.animtime != e->latched.prevanimtime)
		{
			f = (gEngfuncs.GetClientTime() - e->curstate.animtime) / (e->curstate.animtime - e->latched.prevanimtime);
		}
		else
		{
			f = 0;
		}

		for (i = 0; i < 3; i++)
		{
			modelpos[i] -= (e->latched.prevorigin[i] - e->origin[i]) * f;
		}

		if (f != 0.0f && f < 1.5f)
		{
			f = 1.0f - f;

			for (i = 0; i < 3; i++)
			{
				d = e->latched.prevangles[i] - e->angles[i];

				if (d > 180.0)
					d -= 360.0;
				else if (d < -180.0)
					d += 360.0;

				angles[i] += d * f;
			}
		}
	}

	memcpy(matrix, r_identity_matrix, sizeof(r_identity_matrix));
	Matrix4x4_CreateFromEntity(matrix, angles, modelpos, 1);
}

void CPhysicsManager::CreateBarnacle(cl_entity_t *ent)
{
	auto itor = m_staticMap.find(ent->index);

	if (itor != m_staticMap.end())
	{
		return;
	}

	auto staticBody = CreateStaticBody(ent, m_barnacleVertexArray, m_barnacleIndexArray, true);
}

void CPhysicsManager::CreateGargantua(cl_entity_t *ent)
{

}

void CPhysicsManager::CreateBrushModel(cl_entity_t *ent)
{
	int modelindex = EngineGetModelIndex(ent->model);

	if (modelindex == -1)
	{
		//invalid model index?
		g_pMetaHookAPI->SysError("CreateBrushModel: Invalid model index\n");
		return;
	}

	if (!m_brushIndexArray[modelindex])
	{
		//invalid model index?
		g_pMetaHookAPI->SysError("CreateBrushModel: Invalid model index\n");
		return;
	}

	auto itor = m_staticMap.find(ent->index);

	if (itor != m_staticMap.end())
	{
		return;
	}

	bool bKinematic = ((ent != r_worldentity) && (ent->curstate.movetype == MOVETYPE_PUSH || ent->curstate.movetype == MOVETYPE_PUSHSTEP)) ? true : false;

	CreateStaticBody(ent, m_worldVertexArray, m_brushIndexArray[modelindex], bKinematic);
}

void CPhysicsManager::NewMap(void)
{
	G2BScale = bv_scale->value;
	B2GScale = 1 / bv_scale->value;

	ReloadConfig();
	RemoveAllRagdolls();
	RemoveAllStatics();

	r_worldentity = gEngfuncs.GetEntityByIndex(0);

	r_worldmodel = r_worldentity->model;

	GenerateWorldVerticeArray();
	GenerateBrushIndiceArray();
	GenerateBarnacleIndiceVerticeArray();
	GenerateGargantuaIndiceVerticeArray();

	CreateBrushModel(r_worldentity);
}

void CPhysicsManager::GenerateBarnacleIndiceVerticeArray(void)
{
	int BARNACLE_SEGMENTS = 12;

	float BARNACLE_RADIUS1 = 22;
	float BARNACLE_RADIUS2 = 16;
	float BARNACLE_RADIUS3 = 10;

	float BARNACLE_HEIGHT1 = 0;
	float BARNACLE_HEIGHT2 = -10;
	float BARNACLE_HEIGHT3 = -36;

	FloatGoldSrcToBullet(&BARNACLE_RADIUS1);
	FloatGoldSrcToBullet(&BARNACLE_RADIUS2);
	FloatGoldSrcToBullet(&BARNACLE_RADIUS3);
	FloatGoldSrcToBullet(&BARNACLE_HEIGHT1);
	FloatGoldSrcToBullet(&BARNACLE_HEIGHT2);
	FloatGoldSrcToBullet(&BARNACLE_HEIGHT3);

	if (m_barnacleVertexArray)
	{
		delete m_barnacleVertexArray;
		m_barnacleVertexArray = NULL;
	}

	if (m_barnacleIndexArray)
	{
		delete m_barnacleIndexArray;
		m_barnacleIndexArray = NULL;
	}

	m_barnacleVertexArray = new vertexarray_t;
	m_barnacleVertexArray->vVertexBuffer.resize(BARNACLE_SEGMENTS * 8);
	m_barnacleVertexArray->vFaceBuffer.resize(BARNACLE_SEGMENTS * 2);

	int iStartVertex = 0;
	int iNumVerts = 0;
	int iNumFace = 0;

	for (int x = 0; x < BARNACLE_SEGMENTS; x++)
	{
		float xSegment = (float)x / (float)BARNACLE_SEGMENTS;
		float xSegment2 = (float)(x + 1) / (float)BARNACLE_SEGMENTS;

		//layer 1

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * BARNACLE_RADIUS1;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * BARNACLE_RADIUS1;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT1;

		iNumVerts++;

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT2;

		iNumVerts++;

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT2;

		iNumVerts++;

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS1;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS1;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT1;

		iNumVerts++;

		m_barnacleVertexArray->vFaceBuffer[iNumFace].start_vertex = iStartVertex;
		m_barnacleVertexArray->vFaceBuffer[iNumFace].num_vertexes = 4;
		iNumFace++;

		iStartVertex = iNumVerts;

		// layer 2

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT2;

		iNumVerts++;

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * BARNACLE_RADIUS3;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * BARNACLE_RADIUS3;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT3;

		iNumVerts++;

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS3;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS3;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT3;

		iNumVerts++;

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT2;

		iNumVerts++;

		m_barnacleVertexArray->vFaceBuffer[iNumFace].start_vertex = iStartVertex;
		m_barnacleVertexArray->vFaceBuffer[iNumFace].num_vertexes = 4;
		iNumFace++;

		iStartVertex = iNumVerts;
	}

	m_barnacleIndexArray = new indexarray_t;
	for (int i = 0; i < (int)m_barnacleVertexArray->vFaceBuffer.size(); i++)
	{
		GenerateIndexedArrayForBrushface(&m_barnacleVertexArray->vFaceBuffer[i], m_barnacleIndexArray);
	}
}

void CPhysicsManager::GenerateGargantuaIndiceVerticeArray(void)
{
	int GARGANTUA_SEGMENTS = 12;

	float GARGANTUA_RADIUS1 = 16;
	float GARGANTUA_RADIUS2 = 14;
	float GARGANTUA_RADIUS3 = 12;

	float GARGANTUA_HEIGHT1 = 8;
	float GARGANTUA_HEIGHT2 = -8;
	float GARGANTUA_HEIGHT3 = -24;

	FloatGoldSrcToBullet(&GARGANTUA_RADIUS1);
	FloatGoldSrcToBullet(&GARGANTUA_RADIUS2);
	FloatGoldSrcToBullet(&GARGANTUA_RADIUS3);
	FloatGoldSrcToBullet(&GARGANTUA_HEIGHT1);
	FloatGoldSrcToBullet(&GARGANTUA_HEIGHT2);
	FloatGoldSrcToBullet(&GARGANTUA_HEIGHT3);

	if (m_gargantuaVertexArray)
	{
		delete m_gargantuaVertexArray;
		m_gargantuaVertexArray = NULL;
	}

	if (m_gargantuaIndexArray)
	{
		delete m_gargantuaIndexArray;
		m_gargantuaIndexArray = NULL;
	}

	m_gargantuaVertexArray = new vertexarray_t;
	m_gargantuaVertexArray->vVertexBuffer.resize(GARGANTUA_SEGMENTS * (4 + 4));// + 3
	m_gargantuaVertexArray->vFaceBuffer.resize(GARGANTUA_SEGMENTS * 2);

	int iStartVertex = 0;
	int iNumVerts = 0;
	int iNumFace = 0;

	for (int x = 0; x < GARGANTUA_SEGMENTS; x++)
	{
		float xSegment = (float)x / (float)GARGANTUA_SEGMENTS;
		float xSegment2 = (float)(x + 1) / (float)GARGANTUA_SEGMENTS;

		//layer 0, circle

		/*m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = 0;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = 0;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT1;
		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * GARGANTUA_RADIUS1;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * GARGANTUA_RADIUS1;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT1;
		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS1;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS1;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT1;
		iNumVerts++;

		m_gargantuaVertexArray->vFaceBuffer[iNumFace].start_vertex = iStartVertex;
		m_gargantuaVertexArray->vFaceBuffer[iNumFace].num_vertexes = 3;
		iNumFace++;

		iStartVertex = iNumVerts;*/

		//layer 1

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * GARGANTUA_RADIUS1;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * GARGANTUA_RADIUS1;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT1;
		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT2;
		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT2;
		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS1;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS1;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT1;
		iNumVerts++;

		m_gargantuaVertexArray->vFaceBuffer[iNumFace].start_vertex = iStartVertex;
		m_gargantuaVertexArray->vFaceBuffer[iNumFace].num_vertexes = 4;
		iNumFace++;

		iStartVertex = iNumVerts;

		// layer 2

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT2;

		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * GARGANTUA_RADIUS3;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * GARGANTUA_RADIUS3;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT3;

		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS3;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS3;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT3;

		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT2;

		iNumVerts++;

		m_gargantuaVertexArray->vFaceBuffer[iNumFace].start_vertex = iStartVertex;
		m_gargantuaVertexArray->vFaceBuffer[iNumFace].num_vertexes = 4;
		iNumFace++;

		iStartVertex = iNumVerts;
	}

	m_gargantuaIndexArray = new indexarray_t;
	for (int i = 0; i < (int)m_gargantuaVertexArray->vFaceBuffer.size(); i++)
	{
		if (i >= 3 * 2 && i < 8 * 2)
			continue;

		GenerateIndexedArrayForBrushface(&m_gargantuaVertexArray->vFaceBuffer[i], m_gargantuaIndexArray);
	}
}

struct GameFilterCallback : public btOverlapFilterCallback
{
	// return true when pairs need collision
	virtual bool needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const
	{
		bool collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
		collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);

		if (collides)
		{
			auto body0 = (btRigidBody *)proxy0->m_clientObject;
			auto body1 = (btRigidBody *)proxy1->m_clientObject;
			
			if ((proxy0->m_collisionFilterMask & CustomCollisionFilterGroups::RagdollFilter) &&
				(proxy1->m_collisionFilterMask & CustomCollisionFilterGroups::WorldFilter))
			{
				auto physobj0 = (CRigBody *)body0->getUserPointer();
				if (physobj0->flags & RIG_FL_JIGGLE)
				{
					if (!(body0->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT))
					{
						return false;
					}
				}
			}
			else if ((proxy1->m_collisionFilterMask & CustomCollisionFilterGroups::RagdollFilter) &&
				(proxy0->m_collisionFilterMask & CustomCollisionFilterGroups::WorldFilter))
			{
				auto physobj1 = (CRigBody *)body1->getUserPointer();
				if (physobj1->flags & RIG_FL_JIGGLE)
				{
					if (!(body1->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT))
					{
						return false;
					}
				}
			}
		}
		return collides;
	}
};

void CPhysicsManager::Init(void)
{
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_overlappingPairCache = new btDbvtBroadphase();
	m_solver = new btSequentialImpulseConstraintSolver;
	m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_overlappingPairCache, m_solver, m_collisionConfiguration);

	m_debugDraw = new CPhysicsDebugDraw;

	m_dynamicsWorld->setDebugDrawer(m_debugDraw);
	m_dynamicsWorld->setGravity(btVector3(0, 0, 0));

	m_overlapFilterCallback = new GameFilterCallback();
	m_dynamicsWorld->getPairCache()->setOverlapFilterCallback(m_overlapFilterCallback);
}

void CPhysicsManager::DebugDraw(void)
{
	if (bv_debug->value == 1 || bv_debug->value == 4 || bv_debug->value == 5 || bv_debug->value == 6)
	{
		m_dynamicsWorld->debugDrawWorld();
	}
#if 0
	if (bv_debug->value == 2)
	{
		for (auto &ragdoll : m_ragdollMap)
		{
			for (auto &itor : ragdoll.second->m_rigbodyMap)
			{
				auto rig = itor.second;
				if (!(rig->rigbody->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT))
				{
					auto motionState = (BoneMotionState *)rig->rigbody->getMotionState();

					auto vBuoyancy = rig->rigbody->getGravity() * rig->mass;

					vBuoyancy.setZ(vBuoyancy.getZ() * -1);

					auto vResist = rig->rigbody->getLinearVelocity();

					vResist.setX(vResist.getX() * vResist.getX() * -1);
					vResist.setY(vResist.getY() * vResist.getY() * -1);
					vResist.setZ(vResist.getZ() * vResist.getZ() * -1);

					auto bonematrix = motionState->bonematrix;

					for(int i = 0;i < rig->water_control_points.size(); ++i)
					{
						auto &water_control_point = rig->water_control_points[i];

						auto offsetmatrix = motionState->offsetmatrix;

						auto offsetorg = offsetmatrix.getOrigin();

						offsetorg += water_control_point.offset;

						offsetmatrix.setOrigin(offsetorg);

						btTransform worldTrans;
						worldTrans.mult(bonematrix, offsetmatrix);

						auto control_point_origin = worldTrans.getOrigin();

						Vec3BulletToGoldSrc(control_point_origin);

						vec3_t control_point_origin_goldsrc = { control_point_origin.getX(), control_point_origin.getY(), control_point_origin.getZ() };

						vec3_t control_point_origin_goldsrc2 = { control_point_origin.getX(), control_point_origin.getY(), control_point_origin.getZ() + 3 };

						vec3_t control_point_origin_goldsrc3 = { control_point_origin.getX(), control_point_origin.getY() + 3, control_point_origin.getZ() + 3 };

						vec3_t control_point_origin_goldsrc4 = { control_point_origin.getX() + 3, control_point_origin.getY(), control_point_origin.getZ() };

						glDisable(GL_TEXTURE_2D);
						glDisable(GL_BLEND);
						glDisable(GL_DEPTH_TEST);
						glLineWidth(1);

						gEngfuncs.pTriAPI->Color4f(1, 1, 1, 1);
						gEngfuncs.pTriAPI->Begin(TRI_LINES);
						gEngfuncs.pTriAPI->Vertex3fv(control_point_origin_goldsrc);
						gEngfuncs.pTriAPI->Vertex3fv(control_point_origin_goldsrc2);
						gEngfuncs.pTriAPI->End();

						gEngfuncs.pTriAPI->Begin(TRI_LINES);
						gEngfuncs.pTriAPI->Vertex3fv(control_point_origin_goldsrc);
						gEngfuncs.pTriAPI->Vertex3fv(control_point_origin_goldsrc3);
						gEngfuncs.pTriAPI->End();

						gEngfuncs.pTriAPI->Begin(TRI_LINES);
						gEngfuncs.pTriAPI->Vertex3fv(control_point_origin_goldsrc);
						gEngfuncs.pTriAPI->Vertex3fv(control_point_origin_goldsrc4);
						gEngfuncs.pTriAPI->End();

						glEnable(GL_DEPTH_TEST);
						glEnable(GL_BLEND);
						glEnable(GL_TEXTURE_2D);
					}
				}
			}
		}
	}
#endif
}

void CPhysicsManager::ReleaseRagdollFromGargantua(CRagdollBody *ragdoll)
{
	if (ragdoll->m_gargantuaindex != -1)
	{
		ragdoll->m_gargantuaindex = -1;
		ragdoll->m_gargantuaDragRigBody.clear();

		for (auto cst : ragdoll->m_gargantuaConstraintArray)
		{
			m_dynamicsWorld->removeConstraint(cst);
			delete cst;
		}
		ragdoll->m_gargantuaConstraintArray.clear();

		for (auto rig : ragdoll->m_gargantuaDragRigBody)
		{
			rig->gargantua_target = NULL;
		}
	}
}

void CPhysicsManager::ReleaseRagdollFromBarnacle(CRagdollBody *ragdoll)
{
	if (ragdoll->m_barnacleindex != -1)
	{
		ragdoll->m_barnacleindex = -1;

		ragdoll->m_barnacleChewRigBody.clear();

		for (auto cst : ragdoll->m_barnacleConstraintArray)
		{
			m_dynamicsWorld->removeConstraint(cst);
			delete cst;
		}
		ragdoll->m_barnacleConstraintArray.clear();

		for (auto rig : ragdoll->m_barnacleDragRigBody)
		{
			rig->barnacle_constraint_slider = NULL;
			rig->barnacle_constraint_dof6 = NULL;
		}
		ragdoll->m_barnacleDragRigBody.clear();
	}
}

void CPhysicsManager::SyncPlayerView(cl_entity_t *local, struct ref_params_s *pparams)
{
	auto ragdoll = gPhysicsManager.FindRagdoll(local->index);
	if (ragdoll && ragdoll->m_pelvisRigBody)
	{
		auto worldorg = ragdoll->m_pelvisRigBody->rigbody->getWorldTransform().getOrigin();

		Vector3BulletToGoldSrc(worldorg);

		pparams->simorg[0] = worldorg[0];
		pparams->simorg[1] = worldorg[1];
		pparams->simorg[2] = worldorg[2];
	}
}

bool CPhysicsManager::HasRagdolls(void)
{
	return m_ragdollMap.size() ? true : false;
}

bool CPhysicsManager::GetRagdollOrigin(CRagdollBody *ragdoll, float *origin)
{
	auto pelvis = ragdoll->m_pelvisRigBody;
	if (pelvis)
	{
		auto worldtrans = pelvis->rigbody->getWorldTransform();

		auto worldrorg = worldtrans.getOrigin();

		origin[0] = worldrorg.x();
		origin[1] = worldrorg.y();
		origin[2] = worldrorg.z();

		Vec3BulletToGoldSrc(origin);
		return true;
	}
	return false;
}

void CPhysicsManager::UpdateRagdollWaterSimulation(cl_entity_t *ent, CRagdollBody *ragdoll, double frame_time, double client_time)
{
	//Buoyancy and Damping Simulation
#if 1
	for (auto &itor : ragdoll->m_rigbodyMap)
	{
		auto rig = itor.second;

		auto rigbody = rig->rigbody;

		rigbody->setDamping(0, 0);
	
		if (!(rigbody->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT))
		{
			auto motionState = (BoneMotionState *)rigbody->getMotionState();

			auto vBuoyancy = rigbody->getGravity() * rig->mass;

			vBuoyancy.setZ(vBuoyancy.getZ() * -1);

			//btTransform coretrans;
			//motionState->getWorldTransform(coretrans);
			auto coreorigin = rigbody->getCenterOfMassPosition();

			auto bonematrix = motionState->bonematrix;

			for (int i = 0; i < rig->water_control_points.size(); ++i)
			{
				auto &water_control_point = rig->water_control_points[i];

				auto offsetmatrix = motionState->offsetmatrix;

				auto offsetorg = offsetmatrix.getOrigin();

				offsetorg += water_control_point.offset;

				offsetmatrix.setOrigin(offsetorg);

				btTransform worldTrans;
				worldTrans.mult(bonematrix, offsetmatrix);

				auto control_point_origin = worldTrans.getOrigin();

				auto rel_pos = control_point_origin - coreorigin;

				vec3_t control_point_origin_goldsrc = { control_point_origin.getX(), control_point_origin.getY(), control_point_origin.getZ() };

				Vec3BulletToGoldSrc(control_point_origin_goldsrc);

				if (gEngfuncs.PM_PointContents(control_point_origin_goldsrc, NULL) == CONTENT_WATER)
				{
					rigbody->applyForce(vBuoyancy * water_control_point.buoyancy, rel_pos);
					//rigbody->applyCentralForce(vBuoyancy * water_control_point.buoyancy);
					rigbody->setDamping( water_control_point.ldamping, water_control_point.adamping);
				}
			}
		}
	}
#endif
}

bool CPhysicsManager::UpdateRagdoll(cl_entity_t *ent, CRagdollBody *ragdoll, double frame_time, double client_time)
{
	//Don't update if player is not emitted this frame (in firstperson mode)
	if (ent == gEngfuncs.GetLocalPlayer() && !IsEntityEmitted(ent))
		return false;
	
	UpdateRagdollWaterSimulation(ent, ragdoll, frame_time, client_time);

	if (ragdoll->m_gargantuaindex != -1)
	{
		if (StudioGetSequenceActivityType(ent->model, &ent->curstate) != 2)
		{
			ReleaseRagdollFromGargantua(ragdoll);
			
			//Is gibbed ?
			if (ent->curstate.effects & EF_NODRAW)
			{
				return false;
			}

			return true;
		}

		auto gargantua = gEngfuncs.GetEntityByIndex(ragdoll->m_gargantuaindex);

		if (!IsEntityGargantua(gargantua))
		{
			ReleaseRagdollFromGargantua(ragdoll);
			return true;
		}

		if (gargantua->curstate.sequence == 15)
		{
			for (size_t i = 0; i < ragdoll->m_gargantuaDragRigBody.size(); ++i)
			{
				auto rig = ragdoll->m_gargantuaDragRigBody[i];

				if (client_time > rig->gargantua_drag_time)
				{
					btVector3 force = rig->gargantua_target->getWorldTransform().getOrigin() - rig->rigbody->getWorldTransform().getOrigin();
					force.normalize();
					force *= rig->gargantua_force;

					rig->rigbody->applyCentralForce(force);
				}
			}
		}
	}

	else if (ragdoll->m_barnacleindex != -1)
	{
		bool bDraging = true;

		if (StudioGetSequenceActivityType(ent->model, &ent->curstate) != 2)
		{
			ReleaseRagdollFromBarnacle(ragdoll);
			//Is gibbed ?
			if (ent->curstate.effects & EF_NODRAW)
			{
				return false;
			}

			return true;
		}

		auto barnacle = gEngfuncs.GetEntityByIndex(ragdoll->m_barnacleindex);

		if (!IsEntityBarnacle(barnacle))
		{
			ReleaseRagdollFromBarnacle(ragdoll);
			return true;
		}

		if (barnacle->curstate.sequence == 5)
		{
			bDraging = false;

			for (size_t i = 0; i < ragdoll->m_barnacleChewRigBody.size(); ++i)
			{
				auto rig = ragdoll->m_barnacleChewRigBody[i];

				if (client_time > rig->barnacle_chew_time)
				{
					if (rig->barnacle_constraint_dof6 && rig->barnacle_chew_up_z > 0)
					{
						btVector3 currentLimit;
						rig->barnacle_constraint_dof6->getLinearUpperLimit(currentLimit);
						if (currentLimit.x() + rig->barnacle_chew_up_z < rig->barnacle_z_final + 0.01f)
						{
							currentLimit.setX(currentLimit.x() + rig->barnacle_chew_up_z);
							rig->barnacle_constraint_dof6->setLinearUpperLimit(currentLimit);
						}
					}
					else if (rig->barnacle_constraint_slider && rig->barnacle_chew_up_z > 0)
					{
						btScalar currentLimit = rig->barnacle_constraint_slider->getUpperLinLimit();
						if (currentLimit + rig->barnacle_chew_up_z < rig->barnacle_z_final + 0.01f)
						{
							currentLimit = currentLimit + rig->barnacle_chew_up_z;
							rig->barnacle_constraint_slider->setUpperLinLimit(currentLimit);
						}
					}

					if (rig->barnacle_chew_force != 0)
					{
						rig->rigbody->applyCentralImpulse(btVector3(0, 0, rig->barnacle_chew_force));
					}

					rig->barnacle_chew_time = client_time + rig->barnacle_chew_duration;
				}
			}
		}

		vec3_t origin;
		if (GetRagdollOrigin(ragdoll, origin))
		{
			for (size_t i = 0; i < ragdoll->m_barnacleDragRigBody.size(); ++i)
			{
				auto rig = ragdoll->m_barnacleDragRigBody[i];

				btVector3 force(0, 0, rig->barnacle_force);

				if (bDraging)
				{
					if (origin[2] > ent->origin[2] + 24)
						continue;

					if (origin[2] > ent->origin[2])
					{
						force[2] *= (ent->origin[2] + 24 - origin[2]) / 24;
					}
				}

				rig->rigbody->applyCentralForce(force);
			}
		}
	}

	if (ragdoll->m_pelvisRigBody)
	{
		vec3_t origin;
		if (GetRagdollOrigin(ragdoll, origin))
		{
			if (origin[2] < -99999)
				return false;
		}
		//FxDeadPlayer
		if (ent->curstate.effects & EF_NODRAW)
		{
			return true;
		}
	}

	return true;
}

void CPhysicsManager::UpdateTempEntity(TEMPENTITY **ppTempEntActive, double frame_time, double client_time)
{
	//if (!bv_pretick->value)
	//{
		for (auto itor = m_ragdollMap.begin(); itor != m_ragdollMap.end();)
		{
			auto ent = gEngfuncs.GetEntityByIndex(itor->first);

			if (!IsEntityPresent(ent) ||
				!UpdateRagdoll(ent, itor->second, frame_time, client_time))
			{
				itor = FreeRagdollInternal(itor);
			}
			else
			{
				itor++;
			}
		}

		for (auto itor = m_staticMap.begin(); itor != m_staticMap.end();)
		{
			if (itor->first == 0)
			{
				itor++;
				continue;
			}

			auto ent = gEngfuncs.GetEntityByIndex(itor->first);

			if (!IsEntityPresent(ent))
			{
				itor = FreeStaticInternal(itor);
			}
			else
			{
				itor++;
			}
		}
	//}
}

void CPhysicsManager::StepSimulation(double frametime)
{
	if (bv_simrate->value < 32)
	{
		gEngfuncs.Cvar_SetValue("bv_simrate", 32);
	}
	else if (bv_simrate->value > 128)
	{
		gEngfuncs.Cvar_SetValue("bv_simrate", 128);
	}
	m_dynamicsWorld->stepSimulation(frametime, 3, 1.0f / bv_simrate->value);
}

void CPhysicsManager::SetGravity(float velocity)
{
	m_gravity = -velocity;

	FloatGoldSrcToBullet(&m_gravity);

	m_dynamicsWorld->setGravity(btVector3(0, 0, m_gravity));
}

void CPhysicsManager::ReloadConfig(void)
{
	int maxNum = EngineGetMaxKnownModel();

	if ((int)m_ragdoll_config.size() < maxNum)
		m_ragdoll_config.resize(maxNum);

	for (int i = 0; i < maxNum; ++i)
	{
		if (m_ragdoll_config[i])
		{
			delete m_ragdoll_config[i];
			m_ragdoll_config[i] = NULL;
		}
	}

	for (int i = 0; i < EngineGetNumKnownModel(); ++i)
	{
		auto mod = EngineGetModelByIndex(i);
		if (mod->type == mod_studio && mod->name[0])
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			{
				auto moddata = IEngineStudio.Mod_Extradata(mod);
				if (moddata)
				{
					LoadRagdollConfig(mod);
				}
			}
		}
	}
}

ragdoll_config_t *CPhysicsManager::LoadRagdollConfig(model_t *mod)
{
	int modelindex = EngineGetModelIndex(mod);
	if (modelindex == -1)
	{
		//invalid model index?
		g_pMetaHookAPI->SysError("LoadRagdollConfig: Invalid model index\n");
		return NULL;
	}

	auto cfg = m_ragdoll_config[modelindex];

	if (cfg)
		return cfg;

	cfg = new ragdoll_config_t;
	m_ragdoll_config[modelindex] = cfg;

	std::string fullname = mod->name;

	if(fullname.length() < 4)
	{
		g_pMetaHookAPI->SysError("LoadRagdollConfig: Invalid name %s\n", fullname.c_str());
		return NULL;
	}

	auto name = fullname.substr(0, fullname.length() - 4);
	name += "_ragdoll.txt";

	char *pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
	if (!pfile)
	{
		cfg->state = 2;

		gEngfuncs.Con_DPrintf("LoadRagdollConfig: Failed to load config file for %s\n", name.c_str());
		
		return cfg;
	}
	
	cfg->state = 1;

#define RAGDOLL_PARSING_DEATHANIM 0
#define RAGDOLL_PARSING_RIGIDBODY 1
#define RAGDOLL_PARSING_JIGGLEBONE 2
#define RAGDOLL_PARSING_CONSTRAINT 3
#define RAGDOLL_PARSING_BARNACLE 4
#define RAGDOLL_PARSING_GARGANTUA 5
#define RAGDOLL_PARSING_WATERCONTROL 6

	int iParsingState = -1;

	char *ptext = pfile;
	while (1)
	{
		char text[256] = { 0 };

		ptext = gEngfuncs.COM_ParseFile(ptext, text);

		if (!ptext)
			break;

		if (!strcmp(text, "[DeathAnim]"))
		{
			iParsingState = RAGDOLL_PARSING_DEATHANIM;
			continue;
		}
		else if (!strcmp(text, "[RigidBody]"))
		{
			iParsingState = RAGDOLL_PARSING_RIGIDBODY;
			continue;
		}
		else if (!strcmp(text, "[JiggleBone]"))
		{
			iParsingState = RAGDOLL_PARSING_JIGGLEBONE;
			continue;
		}
		else if (!strcmp(text, "[Constraint]"))
		{
			iParsingState = RAGDOLL_PARSING_CONSTRAINT;
			continue;
		}
		else if (!strcmp(text, "[Barnacle]"))
		{
			iParsingState = RAGDOLL_PARSING_BARNACLE;
			continue;
		}
		else if (!strcmp(text, "[Gargantua]"))
		{
			iParsingState = RAGDOLL_PARSING_GARGANTUA;
			continue;
		}
		else if (!strcmp(text, "[WaterControl]"))
		{
			iParsingState = RAGDOLL_PARSING_WATERCONTROL;
			continue;
		}

		std::string subname = text;

		if (iParsingState == RAGDOLL_PARSING_DEATHANIM)
		{
			int i_sequence = atoi(subname.c_str());

			if (i_sequence < 0)
				break;

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
				break;

			float f_frame = atof(text);

			if ((int)cfg->animcontrol.size() < i_sequence + 1)
			{
				cfg->animcontrol.resize(i_sequence + 1);
			}
			cfg->animcontrol[i_sequence].sequence = i_sequence;
			cfg->animcontrol[i_sequence].frame = f_frame;
			cfg->animcontrol[i_sequence].activity = 1;
		}
		else if (iParsingState == RAGDOLL_PARSING_RIGIDBODY || iParsingState == RAGDOLL_PARSING_JIGGLEBONE)
		{
			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse rigidbody boneindex for %s\n", name.c_str());
				break;
			}

			int i_boneindex = atoi(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse rigidbody pboneindex for %s\n", name.c_str());
				break;
			}

			int i_pboneindex = atoi(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse rigidbody shape for %s\n", name.c_str());
				break;
			}

			int i_shape = -1;

			if (!strcmp(text, "sphere"))
			{
				i_shape = RAGDOLL_SHAPE_SPHERE;
			}
			else if (!strcmp(text, "capsule"))
			{
				i_shape = RAGDOLL_SHAPE_CAPSULE;
			}
			else if (!strcmp(text, "gargmouth"))
			{
				i_shape = RAGDOLL_SHAPE_GARGMOUTH;
			}
			else
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse shape name %s for %s\n", text, name.c_str());
				break;
			}

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse rigidbody offset for %s\n", name.c_str());
				break;
			}

			float f_offset = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse rigidbody size for %s\n", name.c_str());
				break;
			}

			float f_size = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse rigidbody size2 for %s\n", name.c_str());
				break;
			}

			float f_size2 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse rigidbody mass for %s\n", name.c_str());
				break;
			}

			float f_mass = atof(text);

			cfg->rigcontrol.emplace_back(subname, i_boneindex, i_pboneindex, i_shape, f_offset, f_offset, f_offset, f_size, f_size2, f_mass, iParsingState == RAGDOLL_PARSING_JIGGLEBONE ? RIG_FL_JIGGLE : 0);
		}
		else if (iParsingState == RAGDOLL_PARSING_CONSTRAINT)
		{
			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
				break;
			
			std::string linktarget = text;

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
				break;

			int i_type = -1;

			if (!strcmp(text, "conetwist"))
			{
				i_type = RAGDOLL_CONSTRAINT_CONETWIST;
			}
			else if (!strcmp(text, "hinge"))
			{
				i_type = RAGDOLL_CONSTRAINT_HINGE;
			}
			else if (!strcmp(text, "point"))
			{
				i_type = RAGDOLL_CONSTRAINT_POINT;
			}
			else
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint type %s for %s\n", text, name.c_str());
				break;
			}

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint boneindex1 for %s\n", name.c_str());
				break;
			}

			int i_boneindex1 = atoi(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint boneindex2 for %s\n", name.c_str());
				break;
			}

			int i_boneindex2 = atoi(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint offset1 for %s\n", name.c_str());
				break;
			}

			float f_offset1 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint offset2 for %s\n", name.c_str());
				break;
			}

			float f_offset2 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint offset3 for %s\n", name.c_str());
				break;
			}

			float f_offset3 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint offset4 for %s\n", name.c_str());
				break;
			}

			float f_offset4 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint offset5 for %s\n", name.c_str());
				break;
			}

			float f_offset5 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint offset6 for %s\n", name.c_str());
				break;
			}

			float f_offset6 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint factor1 for %s\n", name.c_str());
				break;
			}

			float f_factor1 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint factor2 for %s\n", name.c_str());
				break;
			}

			float f_factor2 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint factor3 for %s\n", name.c_str());
				break;
			}

			float f_factor3 = atof(text);

			cfg->cstcontrol.emplace_back(subname, linktarget, i_type, i_boneindex1, i_boneindex2, f_offset1, f_offset2, f_offset3, f_offset4, f_offset5, f_offset6, f_factor1, f_factor2, f_factor3);
		}
		else if (iParsingState == RAGDOLL_PARSING_BARNACLE)
		{
			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle type for %s\n", name.c_str());
				break;
			}

			int i_type = -1;

			if (!strcmp(text, "slider"))
			{
				i_type = RAGDOLL_BARNACLE_SLIDER;
			}
			else if (!strcmp(text, "dof6"))
			{
				i_type = RAGDOLL_BARNACLE_DOF6;
			}
			else if (!strcmp(text, "chewforce"))
			{
				i_type = RAGDOLL_BARNACLE_CHEWFORCE;
			}
			else if (!strcmp(text, "chewlimit"))
			{
				i_type = RAGDOLL_BARNACLE_CHEWLIMIT;
			}
			else
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle type %s for %s\n", text, name.c_str());
				break;
			}

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle offsetX for %s\n", name.c_str());
				break;
			}

			float f_offsetX = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle offsetY for %s\n", name.c_str());
				break;
			}

			float f_offsetY = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle offsetZ for %s\n", name.c_str());
				break;
			}

			float f_offsetZ = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle factor1 for %s\n", name.c_str());
				break;
			}

			float f_factor1 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle factor2 for %s\n", name.c_str());
				break;
			}

			float f_factor2 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle factor3 for %s\n", name.c_str());
				break;
			}

			float f_factor3 = atof(text);

			cfg->barcontrol.emplace_back(subname, f_offsetX, f_offsetY, f_offsetZ, i_type, f_factor1, f_factor2, f_factor3);
		}
		else if (iParsingState == RAGDOLL_PARSING_GARGANTUA)
		{
			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse gargantua link target for %s\n", name.c_str());
				break;
			}

			std::string s_link = text;

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse gargantua type for %s\n", name.c_str());
				break;
			}

			int i_type = -1;

			if (!strcmp(text, "slider"))
			{
				i_type = RAGDOLL_GARGANTUA_SLIDER;
			}
			else if (!strcmp(text, "dof6z"))
			{
				i_type = RAGDOLL_GARGANTUA_DOF6Z;
			}
			else if (!strcmp(text, "dof6"))
			{
				i_type = RAGDOLL_GARGANTUA_DOF6;
			}
			else if (!strcmp(text, "dragforce"))
			{
				i_type = RAGDOLL_GARGANTUA_DRAGFORCE;
			}
			else
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse gargantua type %s for %s\n", text, name.c_str());
				break;
			}

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse gargantua offsetX for %s\n", name.c_str());
				break;
			}

			float f_offsetX = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle offsetY for %s\n", name.c_str());
				break;
			}

			float f_offsetY = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle offsetZ for %s\n", name.c_str());
				break;
			}

			float f_offsetZ = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle factor1 for %s\n", name.c_str());
				break;
			}

			float f_factor1 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle factor2 for %s\n", name.c_str());
				break;
			}

			float f_factor2 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse barnacle factor3 for %s\n", name.c_str());
				break;
			}

			float f_factor3 = atof(text);

			cfg->garcontrol.emplace_back(subname, s_link, f_offsetX, f_offsetY, f_offsetZ, i_type, f_factor1, f_factor2, f_factor3);
		}
		else if (iParsingState == RAGDOLL_PARSING_WATERCONTROL)
		{
			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse water offsetX for %s\n", name.c_str());
				break;
			}

			float f_offsetX = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse water offsetY for %s\n", name.c_str());
				break;
			}

			float f_offsetY = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse water offsetZ for %s\n", name.c_str());
				break;
			}

			float f_offsetZ = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse water factor1 for %s\n", name.c_str());
				break;
			}

			float f_factor1 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse water factor2 for %s\n", name.c_str());
				break;
			}

			float f_factor2 = atof(text);

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse water factor3 for %s\n", name.c_str());
				break;
			}

			float f_factor3 = atof(text);

			cfg->watercontrol.emplace_back(subname, f_offsetX, f_offsetY, f_offsetZ, f_factor1, f_factor2, f_factor3);
		}
	}

	gEngfuncs.COM_FreeFile(pfile);

	return cfg;
}

void BoneMotionState::getWorldTransform(btTransform& worldTrans) const
{
	worldTrans.mult(bonematrix, offsetmatrix);
}

void BoneMotionState::setWorldTransform(const btTransform& worldTrans)
{
	bonematrix.mult(worldTrans, offsetmatrix.inverse());
}

void EulerMatrix(const btVector3& in_euler, btMatrix3x3& out_matrix) {
	btVector3 angles = in_euler;
	angles *= SIMD_RADS_PER_DEG;

	btScalar c1(btCos(angles[0]));
	btScalar c2(btCos(angles[1]));
	btScalar c3(btCos(angles[2]));
	btScalar s1(btSin(angles[0]));
	btScalar s2(btSin(angles[1]));
	btScalar s3(btSin(angles[2]));

	out_matrix.setValue(c1 * c2, -c3 * s2 - s1 * s3 * c2, s3 * s2 - s1 * c3 * c2,
		c1 * s2, c3 * c2 - s1 * s3 * s2, -s3 * c2 - s1 * c3 * s2,
		s1, c1 * s3, c1 * c3);
}

void MatrixEuler(const btMatrix3x3& in_matrix, btVector3& out_euler) {
	out_euler[0] = btAsin(in_matrix[2][0]);

	if (btFabs(in_matrix[2][0]) < (1 - 0.001f)) {
		out_euler[1] = btAtan2(in_matrix[1][0], in_matrix[0][0]);
		out_euler[2] = btAtan2(in_matrix[2][1], in_matrix[2][2]);
	}
	else {
		out_euler[1] = btAtan2(in_matrix[1][2], in_matrix[1][1]);
		out_euler[2] = 0;
	}

	out_euler *= SIMD_DEGS_PER_RAD;
}

void EntityMotionState::getWorldTransform(btTransform& worldTrans) const
{
	btVector3 GoldSrcOrigin(m_ent->curstate.origin[0], m_ent->curstate.origin[1], m_ent->curstate.origin[2]);

	Vector3GoldSrcToBullet(GoldSrcOrigin);

	worldTrans = btTransform(btQuaternion(0, 0, 0, 1), GoldSrcOrigin);

	btVector3 angles(m_ent->curstate.angles[0], m_ent->curstate.angles[1], m_ent->curstate.angles[2]);
	EulerMatrix(angles, worldTrans.getBasis());
}

void EntityMotionState::setWorldTransform(const btTransform& worldTrans)
{
	//wtf?
}

btQuaternion FromToRotaion(btVector3 fromDirection, btVector3 toDirection)
{
	fromDirection = fromDirection.normalize();
	toDirection = toDirection.normalize();

	float cosTheta = fromDirection.dot(toDirection);

	if (cosTheta < -1 + 0.001f) //(Math.Abs(cosTheta)-Math.Abs( -1.0)<1E-6)
	{
		btVector3 up(0.0f, 0.0f, 1.0f);

		auto rotationAxis = up.cross(fromDirection);
		if (rotationAxis.length() < 0.01) // bad luck, they were parallel, try again!
		{
			rotationAxis = up.cross(fromDirection);
		}
		rotationAxis = rotationAxis.normalize();
		return btQuaternion(rotationAxis, (float)M_PI);
	}
	else
	{
		// Implementation from Stan Melax's Game Programming Gems 1 article
		auto rotationAxis = fromDirection.cross(toDirection);

		float s = (float)sqrt((1 + cosTheta) * 2);
		float invs = 1 / s;

		return btQuaternion(
			rotationAxis.x() * invs,
			rotationAxis.y() * invs,
			rotationAxis.z() * invs,
			s * 0.5f
		);
	}
}

btTransform MatrixLookAt(const btTransform &transform, const btVector3 &at, const btVector3 &forward)
{
	auto originVector = forward;
	auto worldToLocalTransform = transform.inverse();

	//transform the target in world position to object's local position
	auto targetVector = worldToLocalTransform * at;

	auto rot = FromToRotaion(originVector, targetVector);
	btTransform rotMatrix = btTransform(rot);

	return transform * rotMatrix;
}

CRigBody *CPhysicsManager::CreateRigBody(studiohdr_t *studiohdr, ragdoll_rig_control_t *rigcontrol)
{
	if (rigcontrol->boneindex >= studiohdr->numbones || rigcontrol->boneindex < 0)
	{
		gEngfuncs.Con_Printf("CreateRigBody: Failed to create rigbody for bone %s, invalid boneindex (%d)\n", rigcontrol->name.c_str(), rigcontrol->boneindex);
		return NULL;
	}

	btTransform bonematrix, offsetmatrix;
	Matrix3x4ToTransform((*pbonetransform)[rigcontrol->boneindex], bonematrix);
	TransformGoldSrcToBullet(bonematrix);

	auto boneorigin = bonematrix.getOrigin();

	btVector3 origin = boneorigin;

	if (rigcontrol->pboneindex >= studiohdr->numbones || rigcontrol->pboneindex < 0)
	{
		gEngfuncs.Con_Printf("CreateRigBody: Failed to create rigbody for bone %s, invalid pboneindex (%d)\n", rigcontrol->name.c_str(), rigcontrol->pboneindex);
		return NULL;
	}

	btVector3 pboneorigin((*pbonetransform)[rigcontrol->pboneindex][0][3], (*pbonetransform)[rigcontrol->pboneindex][1][3], (*pbonetransform)[rigcontrol->pboneindex][2][3]);
	Vector3GoldSrcToBullet(pboneorigin);

	btVector3 dir = pboneorigin - boneorigin;
	dir = dir.normalize();

	btVector3 offset = btVector3(rigcontrol->offset[0], rigcontrol->offset[1], rigcontrol->offset[2]);
	Vector3GoldSrcToBullet(offset);

	origin += dir * offset;

	if (rigcontrol->shape == RAGDOLL_SHAPE_SPHERE)
	{
		btTransform rigidtransform;
		rigidtransform.setIdentity();
		rigidtransform.setOrigin(origin);
		offsetmatrix.mult(bonematrix.inverse(), rigidtransform);

		float rigsize = rigcontrol->size;
		FloatGoldSrcToBullet(&rigsize);

		auto shape = new btSphereShape(rigsize);

		BoneMotionState* motionState = new BoneMotionState(bonematrix, offsetmatrix);
		
		float mass = rigcontrol->mass;

		btVector3 localInertia;
		shape->calculateLocalInertia(mass, localInertia);

		btRigidBody::btRigidBodyConstructionInfo cInfo(mass, motionState, shape, localInertia);

		auto rig = new CRigBody;

		rig->name = rigcontrol->name;
		rig->rigbody = new btRigidBody(cInfo);
		rig->boneindex = rigcontrol->boneindex;
		rig->flags = rigcontrol->flags;
		rig->mass = mass;

		rig->rigbody->setUserPointer(rig);

		m_dynamicsWorld->addRigidBody(rig->rigbody, rig->group, rig->mask);

		return rig;
	}
	else if (rigcontrol->shape == RAGDOLL_SHAPE_CAPSULE)
	{
		float rigsize = rigcontrol->size;
		FloatGoldSrcToBullet(&rigsize);

		float rigsize2 = rigcontrol->size2;
		FloatGoldSrcToBullet(&rigsize2);

		auto bonematrix2 = bonematrix;
		bonematrix2.setOrigin(origin);
		
		btVector3 fwd(0, 1, 0);
		auto rigidtransform = MatrixLookAt(bonematrix2, pboneorigin, fwd);
		offsetmatrix.mult(bonematrix.inverse(), rigidtransform);

		auto shape = new btCapsuleShape(rigsize, rigsize2);

		BoneMotionState* motionState = new BoneMotionState(bonematrix, offsetmatrix);

		float mass = rigcontrol->mass;

		btVector3 localInertia;
		shape->calculateLocalInertia(mass, localInertia);

		btRigidBody::btRigidBodyConstructionInfo cInfo(mass, motionState, shape, localInertia);

		auto rig = new CRigBody;

		rig->name = rigcontrol->name;
		rig->rigbody = new btRigidBody(cInfo);
		rig->boneindex = rigcontrol->boneindex;
		rig->flags = rigcontrol->flags;
		rig->mass = mass;

		rig->rigbody->setUserPointer(rig);

		m_dynamicsWorld->addRigidBody(rig->rigbody, rig->group, rig->mask);

		return rig;
	}
	else if (rigcontrol->shape == RAGDOLL_SHAPE_GARGMOUTH)
	{
		float rigsize = rigcontrol->size;
		FloatGoldSrcToBullet(&rigsize);

		float rigsize2 = rigcontrol->size2;
		FloatGoldSrcToBullet(&rigsize2);

		auto bonematrix2 = bonematrix;
		bonematrix2.setOrigin(origin);

		btVector3 fwd(0, 0, 1);
		auto rigidtransform = MatrixLookAt(bonematrix2, pboneorigin, fwd);
		offsetmatrix.mult(bonematrix.inverse(), rigidtransform);

		auto vertexArray = new btTriangleIndexVertexArray(
			m_gargantuaIndexArray->vIndiceBuffer.size() / 3, m_gargantuaIndexArray->vIndiceBuffer.data(), 3 * sizeof(int),
			m_gargantuaVertexArray->vVertexBuffer.size(), (float *)m_gargantuaVertexArray->vVertexBuffer.data(), sizeof(brushvertex_t));

		float mass = rigcontrol->mass;

		btRigidBody::btRigidBodyConstructionInfo cInfo(mass, new BoneMotionState(bonematrix, offsetmatrix), new btBvhTriangleMeshShape(vertexArray, true, true));

		auto rig = new CRigBody;

		rig->name = rigcontrol->name;
		rig->rigbody = new btRigidBody(cInfo);
		rig->boneindex = rigcontrol->boneindex;
		rig->flags = rigcontrol->flags | RIG_FL_KINEMATIC;
		rig->mass = mass;

		rig->rigbody->setUserPointer(rig);

		m_dynamicsWorld->addRigidBody(rig->rigbody, rig->group, rig->mask);

		return rig;
	}

	gEngfuncs.Con_Printf("CreateRigBody: Failed to create rigbody %s, invalid shape type %d\n", rigcontrol->name.c_str(), rigcontrol->shape);
	return NULL;
}
#if 0
class BuoyancyAction : public btActionInterface
{
public:
	BuoyancyAction(CRigBody *body)
	{
		m_body = body;
	}

	BuoyancyAction::~BuoyancyAction()
	{
		
	}

	void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) final
	{
		m_body->rigbody->setDamping(0, 0);

		if (!(m_body->rigbody->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT))
		{
			auto motionState = (BoneMotionState *)m_body->rigbody->getMotionState();

			auto vBuoyancy = m_body->rigbody->getGravity() * m_body->mass;

			vBuoyancy.setZ(vBuoyancy.getZ() * -1);

			auto bonematrix = motionState->bonematrix;

			for (int i = 0; i < m_body->water_control_points.size(); ++i)
			{
				auto &water_control_point = m_body->water_control_points[i];

				auto offsetmatrix = motionState->offsetmatrix;

				auto offsetorg = offsetmatrix.getOrigin();

				offsetorg += water_control_point.offset;

				offsetmatrix.setOrigin(offsetorg);

				btTransform worldTrans;
				worldTrans.mult(bonematrix, offsetmatrix);

				auto control_point_origin = worldTrans.getOrigin();

				vec3_t control_point_origin_goldsrc = { control_point_origin.getX(), control_point_origin.getY(), control_point_origin.getZ() };

				Vec3BulletToGoldSrc(control_point_origin_goldsrc);

				int content = gEngfuncs.PM_PointContents(control_point_origin_goldsrc, NULL);
				if (content == CONTENT_WATER)
				{
					m_body->rigbody->applyForce(vBuoyancy * water_control_point.buoyancy, );
					m_body->rigbody->setDamping(water_control_point.ldamping, water_control_point.adamping);
				}
			}
		}
	}

	void debugDraw(btIDebugDraw* debugDrawer) final
	{
		//btTransform tr(btQuaternion::getIdentity(), targetPoint);
		//debugDrawer->drawTransform(tr, 1.f);
	}

	CRigBody *m_body;
};
#endif

void CPhysicsManager::CreateWaterControl(CRagdollBody *ragdoll, studiohdr_t *studiohdr, ragdoll_water_control_t *water_control)
{
	auto itor = ragdoll->m_rigbodyMap.find(water_control->name);
	if (itor == ragdoll->m_rigbodyMap.end())
	{
		gEngfuncs.Con_Printf("CreateWaterControl: Failed to create water control point, rigidbody %s not found\n", water_control->name.c_str());
		return;
	}

	auto Body = itor->second;

	btVector3 offset(water_control->offsetX, water_control->offsetY, water_control->offsetZ);

	Vec3GoldSrcToBullet(offset);

	Body->water_control_points.push_back( CWaterControlPoint(offset, water_control->factor1, water_control->factor2, water_control->factor3) );
}

btTypedConstraint *CPhysicsManager::CreateConstraint(CRagdollBody *ragdoll, studiohdr_t *studiohdr, ragdoll_cst_control_t *cstcontrol)
{
	auto itor = ragdoll->m_rigbodyMap.find(cstcontrol->name);
	if (itor == ragdoll->m_rigbodyMap.end())
	{
		gEngfuncs.Con_Printf("CreateConstraint: Failed to create constraint, rigidbody %s not found\n", cstcontrol->name.c_str());
		return NULL;
	}
	auto itor2 = ragdoll->m_rigbodyMap.find(cstcontrol->linktarget);
	if (itor2 == ragdoll->m_rigbodyMap.end())
	{
		gEngfuncs.Con_Printf("CreateConstraint: Failed to create constraint, linked rigidbody %s not found\n", cstcontrol->linktarget.c_str());
		return NULL;
	}

	if (cstcontrol->boneindex1 >= studiohdr->numbones)
	{
		gEngfuncs.Con_Printf("CreateConstraint: Failed to create constraint for rigidbody %s, boneindex1 too large (%d >= %d)\n", cstcontrol->name.c_str(), cstcontrol->boneindex1, studiohdr->numbones);
		return NULL;
	}

	if (cstcontrol->boneindex2 >= studiohdr->numbones)
	{
		gEngfuncs.Con_Printf("CreateConstraint: Failed to create constraint for rigidbody %s, boneindex2 too large (%d >= %d)\n", cstcontrol->name.c_str(), cstcontrol->boneindex2, studiohdr->numbones);
		return NULL;
	}

	btTransform bonematrix1;
	Matrix3x4ToTransform((*pbonetransform)[cstcontrol->boneindex1], bonematrix1);
	TransformGoldSrcToBullet(bonematrix1);

	btTransform bonematrix2;
	Matrix3x4ToTransform((*pbonetransform)[cstcontrol->boneindex2], bonematrix2);
	TransformGoldSrcToBullet(bonematrix2);

	float offset1 = cstcontrol->offset1;
	float offset2 = cstcontrol->offset2;
	float offset3 = cstcontrol->offset3;
	float offset4 = cstcontrol->offset4;
	float offset5 = cstcontrol->offset5;
	float offset6 = cstcontrol->offset6;

	FloatGoldSrcToBullet(&offset1);
	FloatGoldSrcToBullet(&offset2);
	FloatGoldSrcToBullet(&offset3);
	FloatGoldSrcToBullet(&offset4);
	FloatGoldSrcToBullet(&offset5);
	FloatGoldSrcToBullet(&offset6);

	auto rig1 = itor->second;
	auto rig2 = itor2->second;

	if (cstcontrol->type == RAGDOLL_CONSTRAINT_CONETWIST)
	{
		auto trans1 = rig1->rigbody->getWorldTransform();
		auto trans2 = rig2->rigbody->getWorldTransform();

		auto inv1 = trans1.inverse();
		auto inv2 = trans2.inverse();

		btTransform localrig1;
		localrig1.mult(inv1, bonematrix1);
		localrig1.setOrigin(btVector3(offset1, offset2, offset3));

		btTransform localrig2;
		localrig2.mult(inv2, bonematrix2);
		localrig2.setOrigin(btVector3(offset4, offset5, offset6));

		if (offset1 == 0 && offset1 == 0 && offset3 == 0 && !(offset4 == 0 && offset5 == 0 && offset6 == 0))
		{
			btTransform globaljoint;
			globaljoint.mult(trans2, localrig2);

			btTransform localrig1_org;
			localrig1_org.mult(inv1, globaljoint);

			localrig1.setOrigin(localrig1_org.getOrigin());
		}
		else if (offset4 == 0 && offset5 == 0 && offset6 == 0 && !(offset1 == 0 && offset1 == 0 && offset3 == 0))
		{
			btTransform globaljoint;
			globaljoint.mult(trans1, localrig1);

			btTransform localrig2_org;
			localrig2_org.mult(inv2, globaljoint);

			localrig2.setOrigin(localrig2_org.getOrigin());
		}

		auto cst = new btConeTwistConstraint(*rig1->rigbody, *rig2->rigbody, localrig1, localrig2);

		if (bv_debug->value == 4 && (rig1->rigbody->getMass() != 0 || rig2->rigbody->getMass() != 0))
		{
			cst->setDbgDrawSize(4);
		}
		else
		{
			cst->setDbgDrawSize(1);
		}
		
		cst->setLimit(cstcontrol->factor1 * M_PI, cstcontrol->factor2 * M_PI, cstcontrol->factor3 * M_PI, 1, 1, 1);

		return cst;
	}

	else if (cstcontrol->type == RAGDOLL_CONSTRAINT_HINGE)
	{
		auto trans1 = rig1->rigbody->getWorldTransform();
		auto trans2 = rig2->rigbody->getWorldTransform();

		auto inv1 = trans1.inverse();
		auto inv2 = trans2.inverse();

		btTransform localrig1;
		localrig1.mult(inv1, bonematrix1);
		localrig1.setOrigin(btVector3(offset1, offset2, offset3));

		btTransform localrig2;
		localrig2.mult(inv2, bonematrix2);
		localrig2.setOrigin(btVector3(offset4, offset5, offset6));

		if (offset1 == 0 && offset1 == 0 && offset3 == 0 && !(offset4 == 0 && offset5 == 0 && offset6 == 0))
		{
			btTransform globaljoint;
			globaljoint.mult(trans2, localrig2);

			btTransform localrig1_org;
			localrig1_org.mult(inv1, globaljoint);

			localrig1.setOrigin(localrig1_org.getOrigin());
		}
		else if (offset4 == 0 && offset5 == 0 && offset6 == 0 && !(offset1 == 0 && offset1 == 0 && offset3 == 0))
		{
			btTransform globaljoint;
			globaljoint.mult(trans1, localrig1);

			btTransform localrig2_org;
			localrig2_org.mult(inv2, globaljoint);

			localrig2.setOrigin(localrig2_org.getOrigin());
		}

		auto cst = new btHingeConstraint(*rig1->rigbody, *rig2->rigbody, localrig1, localrig2);
	
		if (bv_debug->value == 5 && (rig1->rigbody->getMass() != 0 || rig2->rigbody->getMass() != 0))
		{
			cst->setDbgDrawSize(4);
		}
		else
		{
			cst->setDbgDrawSize(1);
		}

		cst->setLimit(cstcontrol->factor1 * M_PI, cstcontrol->factor2 * M_PI, 0.1f);
		return cst;
	}
	else if (cstcontrol->type == RAGDOLL_CONSTRAINT_POINT)
	{
		auto trans1 = rig1->rigbody->getWorldTransform();
		auto trans2 = rig2->rigbody->getWorldTransform();

		auto inv1 = trans1.inverse();
		auto inv2 = trans2.inverse();

		btTransform localrig1;
		localrig1.mult(inv1, bonematrix1);
		localrig1.setOrigin(btVector3(offset1, offset2, offset3));

		btTransform localrig2;
		localrig2.mult(inv2, bonematrix2);
		localrig2.setOrigin(btVector3(offset4, offset5, offset6));

		auto cst = new btPoint2PointConstraint(*rig1->rigbody, *rig2->rigbody, localrig1.getOrigin(), localrig2.getOrigin());

		if (bv_debug->value == 6 && (rig1->rigbody->getMass() != 0 || rig2->rigbody->getMass() != 0))
		{
			cst->setDbgDrawSize(1);
		}
		else
		{
			cst->setDbgDrawSize(0.25f);
		}

		return cst;
	}

	gEngfuncs.Con_Printf("CreateConstraint: Failed to create constraint for %s, invalid type %d\n", cstcontrol->name.c_str(), cstcontrol->type);
	return NULL;
}

void CPhysicsManager::RemoveAllStatics()
{
	for (auto p : m_staticMap)
	{
		auto staticBody = p.second;
		if (staticBody)
		{
			if (staticBody->m_rigbody)
			{
				m_dynamicsWorld->removeRigidBody(staticBody->m_rigbody);
				delete staticBody->m_rigbody;
			}
			if (staticBody->m_vertexarray && staticBody->m_vertexarray->bIsDynamic)
			{
				delete staticBody->m_vertexarray;
			}
			if (staticBody->m_indexarray && staticBody->m_indexarray->bIsDynamic)
			{
				delete staticBody->m_indexarray;
			}

			delete staticBody; 
		}
	}

	m_staticMap.clear();
}

void CPhysicsManager::RemoveAllRagdolls()
{
	for (auto itor = m_ragdollMap.begin(); itor != m_ragdollMap.end(); )
	{
		itor = FreeRagdollInternal(itor);
	}
}

ragdoll_itor CPhysicsManager::FreeRagdollInternal(ragdoll_itor &itor)
{
	auto ragdoll = itor->second;

	ReleaseRagdollFromBarnacle(ragdoll);
	ReleaseRagdollFromGargantua(ragdoll);

	RagdollDestroyCallback(ragdoll->m_entindex);

	for (auto p : ragdoll->m_constraintArray)
	{
		m_dynamicsWorld->removeConstraint(p);
		delete p;
	}

	ragdoll->m_constraintArray.clear();

	for (auto p : ragdoll->m_rigbodyMap)
	{
		if (p.second->buoyancy)
		{
			m_dynamicsWorld->removeAction(p.second->buoyancy);
			delete p.second->buoyancy;
		}
		if (p.second->rigbody)
		{
			m_dynamicsWorld->removeRigidBody(p.second->rigbody);
			delete p.second;
		}
	}

	ragdoll->m_rigbodyMap.clear();

	delete ragdoll;

	return m_ragdollMap.erase(itor);
}

static_itor CPhysicsManager::FreeStaticInternal(static_itor &itor)
{
	auto staticBody = itor->second;
	
	if (staticBody)
	{
		if (staticBody->m_rigbody)
		{
			m_dynamicsWorld->removeRigidBody(staticBody->m_rigbody);
			delete staticBody->m_rigbody;
		}
		if (staticBody->m_vertexarray && staticBody->m_vertexarray->bIsDynamic)
		{
			delete staticBody->m_vertexarray;
		}
		if (staticBody->m_indexarray && staticBody->m_indexarray->bIsDynamic)
		{
			delete staticBody->m_indexarray;
		}

		delete staticBody;
	}
	itor->second = NULL;

	return m_staticMap.erase(itor);
}

void CPhysicsManager::RemoveRagdollEx(ragdoll_itor &itor)
{
	if (itor == m_ragdollMap.end())
	{
		gEngfuncs.Con_Printf("RemoveRagdollEx: not found\n");
		return;
	}

	FreeRagdollInternal(itor);
}

void CPhysicsManager::RemoveRagdoll(int tentindex)
{
	RemoveRagdollEx(m_ragdollMap.find(tentindex));
}

void CPhysicsManager::MergeBarnacleBones(studiohdr_t *hdr, int entindex)
{
	auto itor = m_ragdollMap.find(entindex);

	if (itor == m_ragdollMap.end())
	{
		return;
	}

	mstudiobone_t *pbones = (mstudiobone_t *)((byte *)(*pstudiohdr) + (*pstudiohdr)->boneindex);

	auto ragdoll = itor->second;

	if (ragdoll->m_barnacleDragRigBody.size() == 0)
		return;

	auto dragrig = ragdoll->m_barnacleDragRigBody[0];

	auto worldtrans = dragrig->rigbody->getWorldTransform();

	auto inv = worldtrans.inverse();

	btTransform localrig;
	localrig.setIdentity();
	localrig.setOrigin(dragrig->barnacle_drag_offset);

	btTransform worldtrans2;
	worldtrans2.mult(worldtrans, localrig);

	auto rigorgin = worldtrans2.getOrigin();
	
	Vector3BulletToGoldSrc(rigorgin);

	for (int i = 11; i <= 16; ++i)
	{
		(*pbonetransform)[i][0][3] = rigorgin.x();
		(*pbonetransform)[i][1][3] = rigorgin.y();
		(*pbonetransform)[i][2][3] = rigorgin.z() + 8;
	}
}

bool CPhysicsManager::SetupJiggleBones(studiohdr_t *hdr, int entindex)
{
	auto itor = m_ragdollMap.find(entindex);

	if (itor == m_ragdollMap.end())
	{
		return false;
	}

	mstudiobone_t *pbones = (mstudiobone_t *)((byte *)(*pstudiohdr) + (*pstudiohdr)->boneindex);

	auto ragdoll = itor->second;

	for (auto &p : ragdoll->m_rigbodyMap)
	{
		auto rig = p.second;

		auto motionState = (BoneMotionState *)rig->rigbody->getMotionState();
	
		if ((rig->flags & RIG_FL_JIGGLE) && !(rig->flags & RIG_FL_KINEMATIC) && !ragdoll->m_bUpdateKinematic )
		{
			//Dynamic rigs

			auto bonematrix = motionState->bonematrix;

			TransformBulletToGoldSrc(bonematrix);

			float bonematrix_3x4[3][4];
			TransformToMatrix3x4(bonematrix, bonematrix_3x4);

			memcpy((*pbonetransform)[rig->boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
			memcpy((*plighttransform)[rig->boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
		}
		else
		{
			//Kinematic rigs

			auto &bonematrix = motionState->bonematrix;

			Matrix3x4ToTransform((*pbonetransform)[rig->boneindex], bonematrix);
			TransformGoldSrcToBullet(bonematrix);
		}
	}

	return true;
}

bool CPhysicsManager::SetupBones(studiohdr_t *hdr, int entindex)
{
	auto itor = m_ragdollMap.find(entindex);

	if (itor == m_ragdollMap.end())
	{
		return false;
	}

	mstudiobone_t *pbones = (mstudiobone_t *)((byte *)(*pstudiohdr) + (*pstudiohdr)->boneindex);

	auto ragdoll = itor->second;

	for (auto &p : ragdoll->m_rigbodyMap)
	{
		auto rig = p.second;

		auto motionState = (BoneMotionState *)rig->rigbody->getMotionState();

		auto bonematrix = motionState->bonematrix;

		TransformBulletToGoldSrc(bonematrix);

		float bonematrix_3x4[3][4];
		TransformToMatrix3x4(bonematrix, bonematrix_3x4);

		memcpy((*pbonetransform)[rig->boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
		memcpy((*plighttransform)[rig->boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
	}

	for (size_t index = 0; index < ragdoll->m_nonKeyBones.size(); index++)
	{
		auto i = ragdoll->m_nonKeyBones[index];
		if (i == -1)
			continue;

		auto parentmatrix3x4 = (*pbonetransform)[pbones[i].parent];
		
		btTransform parentmatrix;
		Matrix3x4ToTransform(parentmatrix3x4, parentmatrix);

		btTransform mergedmatrix;
		mergedmatrix = parentmatrix * ragdoll->m_boneRelativeTransform[i];

		TransformToMatrix3x4(mergedmatrix, (*pbonetransform)[i]);
	}

	return true;
}

bool CPhysicsManager::IsValidRagdoll(ragdoll_itor &itor)
{
	return (itor != m_ragdollMap.end()) ? true : false;
}

ragdoll_itor CPhysicsManager::FindRagdollEx(int tentindex)
{
	return m_ragdollMap.find(tentindex);
}

CRagdollBody *CPhysicsManager::FindRagdoll(int tentindex)
{
	auto itor = FindRagdollEx(tentindex);

	if (itor != m_ragdollMap.end())
	{
		return itor->second;
	}

	return NULL;
}

void CPhysicsManager::ResetPose(CRagdollBody *ragdoll, entity_state_t *curstate)
{
	bool bNeedResetKinematic = false;

	mstudiobone_t *pbones = (mstudiobone_t *)((byte *)ragdoll->m_studiohdr + ragdoll->m_studiohdr->boneindex);

	for (auto &p : ragdoll->m_rigbodyMap)
	{
		auto rig = p.second;

		if (rig->flags & RIG_FL_JIGGLE)
		{
			auto motionState = (BoneMotionState *)rig->rigbody->getMotionState();

			auto &bonematrix = motionState->bonematrix;

			Matrix3x4ToTransform((*pbonetransform)[rig->boneindex], bonematrix);
			TransformGoldSrcToBullet(bonematrix);

			rig->rigbody->setAngularVelocity(btVector3(0, 0, 0));
			rig->rigbody->setLinearVelocity(btVector3(0, 0, 0));

			//Transform to dynamic at next tick
			rig->rigbody->setCollisionFlags(rig->oldCollisionFlags | btCollisionObject::CF_KINEMATIC_OBJECT);
			rig->rigbody->setActivationState(DISABLE_DEACTIVATION);

			bNeedResetKinematic = true;
		}
	}

	if (bNeedResetKinematic)
	{
		ragdoll->m_flUpdateKinematicTime = curstate->msg_time + 0.05f;
		ragdoll->m_bUpdateKinematic = true;
	}
}

void CPhysicsManager::ApplyGargantua(CRagdollBody *ragdoll, cl_entity_t *gargantuaEntity)
{
	auto gargRagdollItor = FindRagdollEx(gargantuaEntity->index);

	if (gargRagdollItor == m_ragdollMap.end())
	{
		return;
	}

	auto gargRagdollBody = gargRagdollItor->second;

	ragdoll->m_gargantuaindex = gargantuaEntity->index;

	for (auto &p : ragdoll->m_rigbodyMap)
	{
		auto rig = p.second;

		rig->rigbody->setLinearVelocity(btVector3(0, 0, 0));
		rig->rigbody->setAngularVelocity(btVector3(0, 0, 0));

		for (size_t j = 0; j < ragdoll->m_garcontrol.size(); ++j)
		{
			auto garcontrol = &ragdoll->m_garcontrol[j];

			if (garcontrol->name == rig->name)
			{
				if (garcontrol->type == RAGDOLL_GARGANTUA_SLIDER)
				{
					auto linkTarget = gargRagdollBody->m_rigbodyMap.find(garcontrol->name2);

					if (linkTarget != gargRagdollBody->m_rigbodyMap.end())
					{
						
					}
				}
				else if (garcontrol->type == RAGDOLL_GARGANTUA_DRAGFORCE)
				{
					auto linkTarget = gargRagdollBody->m_rigbodyMap.find(garcontrol->name2);

					if (linkTarget != gargRagdollBody->m_rigbodyMap.end())
					{
						if (std::find(ragdoll->m_gargantuaDragRigBody.begin(), ragdoll->m_gargantuaDragRigBody.end(), rig) == ragdoll->m_gargantuaDragRigBody.end())
							ragdoll->m_gargantuaDragRigBody.emplace_back(rig);

						auto linkTargetRigbody = linkTarget->second;

						float factor1 = garcontrol->factor1;
						FloatGoldSrcToBullet(&factor1);

						btVector3 offset(garcontrol->offsetX, garcontrol->offsetY, garcontrol->offsetZ);
						Vector3GoldSrcToBullet(offset);

						rig->gargantua_drag_offset = offset;
						rig->gargantua_force = factor1;
						rig->gargantua_target = linkTargetRigbody->rigbody;
						rig->gargantua_drag_time = gEngfuncs.GetClientTime() + garcontrol->factor2;
					}
				}
				else if (garcontrol->type == RAGDOLL_GARGANTUA_DOF6Z)
				{
					auto linkTarget = gargRagdollBody->m_rigbodyMap.find(garcontrol->name2);

					if (linkTarget != gargRagdollBody->m_rigbodyMap.end())
					{
						if (std::find(ragdoll->m_gargantuaDragRigBody.begin(), ragdoll->m_gargantuaDragRigBody.end(), rig) == ragdoll->m_gargantuaDragRigBody.end())
							ragdoll->m_gargantuaDragRigBody.emplace_back(rig);

						auto linkTargetRigbody = linkTarget->second;

						btTransform rigtrans = rig->rigbody->getWorldTransform();

						btTransform gargtrans = linkTargetRigbody->rigbody->getWorldTransform();

						btTransform localrig1;
						localrig1.setIdentity();
						float factor2 = garcontrol->factor2;
						btVector3 offset1(0, 0, factor2);
						Vector3GoldSrcToBullet(offset1);
						localrig1.setOrigin(offset1);

						btTransform localrig2;
						localrig2.setIdentity();
						btVector3 offset2(garcontrol->offsetX, garcontrol->offsetY, garcontrol->offsetZ);
						Vector3GoldSrcToBullet(offset2);
						localrig2.setOrigin(offset2);

						rig->gargantua_drag_offset = offset2;

						auto constraint = new btGeneric6DofConstraint(*linkTargetRigbody->rigbody, *rig->rigbody, localrig1, localrig2, true);

						auto distance = gargtrans.getOrigin().distance(rigtrans.getOrigin());

						float factor3 = garcontrol->factor3;
						FloatGoldSrcToBullet(&factor3);

						constraint->setAngularLowerLimit(btVector3(M_PI * -1, M_PI * -1, M_PI * -1));
						constraint->setAngularUpperLimit(btVector3(M_PI * 1, M_PI * 1, M_PI * 1));
						constraint->setLinearLowerLimit(btVector3(0, 0, 0));
						constraint->setLinearUpperLimit(btVector3(0, 0, factor3));
						constraint->setDbgDrawSize(5);

						ragdoll->m_gargantuaConstraintArray.emplace_back(constraint);

						m_dynamicsWorld->addConstraint(constraint);

						float factor1 = garcontrol->factor1;
						FloatGoldSrcToBullet(&factor1);

						rig->gargantua_force = factor1;
						rig->gargantua_target = linkTargetRigbody->rigbody;
						rig->gargantua_drag_time = gEngfuncs.GetClientTime();
					}
				}
				else if (garcontrol->type == RAGDOLL_GARGANTUA_DOF6)
				{
					auto linkTarget = gargRagdollBody->m_rigbodyMap.find(garcontrol->name2);

					if (linkTarget != gargRagdollBody->m_rigbodyMap.end())
					{
						if (std::find(ragdoll->m_gargantuaDragRigBody.begin(), ragdoll->m_gargantuaDragRigBody.end(), rig) == ragdoll->m_gargantuaDragRigBody.end())
							ragdoll->m_gargantuaDragRigBody.emplace_back(rig);

						auto linkTargetRigbody = linkTarget->second;

						btTransform rigtrans = rig->rigbody->getWorldTransform();

						btTransform gargtrans = linkTargetRigbody->rigbody->getWorldTransform();

						btTransform localrig1;
						localrig1.setIdentity();
						float factor2 = garcontrol->factor2;
						btVector3 offset1(0, 0, factor2);
						Vector3GoldSrcToBullet(offset1);
						localrig1.setOrigin(offset1);

						btTransform localrig2;
						localrig2.setIdentity();
						btVector3 offset2(garcontrol->offsetX, garcontrol->offsetY, garcontrol->offsetZ);
						Vector3GoldSrcToBullet(offset2);
						localrig2.setOrigin(offset2);

						rig->gargantua_drag_offset = offset2;

						auto constraint = new btGeneric6DofConstraint(*linkTargetRigbody->rigbody, *rig->rigbody, localrig1, localrig2, true);

						auto distance = gargtrans.getOrigin().distance(rigtrans.getOrigin());

						float factor3 = garcontrol->factor3;
						FloatGoldSrcToBullet(&factor3);

						constraint->setAngularLowerLimit(btVector3(M_PI * -1, M_PI * -1, M_PI * -1));
						constraint->setAngularUpperLimit(btVector3(M_PI * 1, M_PI * 1, M_PI * 1));
						constraint->setLinearLowerLimit(btVector3(0, 0, 0));
						constraint->setLinearUpperLimit(btVector3(factor3, 0, 0));
						constraint->setDbgDrawSize(5);

						ragdoll->m_gargantuaConstraintArray.emplace_back(constraint);

						m_dynamicsWorld->addConstraint(constraint);

						float factor1 = garcontrol->factor1;
						FloatGoldSrcToBullet(&factor1);

						rig->gargantua_force = factor1;
						rig->gargantua_target = linkTargetRigbody->rigbody;
						rig->gargantua_drag_time = gEngfuncs.GetClientTime();
					}
				}
			}
		}
	}
}

void CPhysicsManager::ApplyBarnacle(CRagdollBody *ragdoll, cl_entity_t *barnacleEntity)
{
	ragdoll->m_barnacleindex = barnacleEntity->index;

	for (auto &p : ragdoll->m_rigbodyMap)
	{
		auto rig = p.second;

		rig->rigbody->setLinearVelocity(btVector3(0, 0, 0));
		rig->rigbody->setAngularVelocity(btVector3(0, 0, 0));

		for (size_t j = 0; j < ragdoll->m_barcontrol.size(); ++j)
		{
			auto barcontrol = &ragdoll->m_barcontrol[j];

			if (barcontrol->name == rig->name)
			{
				if (barcontrol->type == RAGDOLL_BARNACLE_SLIDER)
				{
					if (std::find(ragdoll->m_barnacleDragRigBody.begin(), ragdoll->m_barnacleDragRigBody.end(), rig) == ragdoll->m_barnacleDragRigBody.end())
						ragdoll->m_barnacleDragRigBody.emplace_back(rig);

					btVector3 fwd(1, 0, 0);

					btTransform rigtrans = rig->rigbody->getWorldTransform();

					btVector3 barnacle_origin(barnacleEntity->origin[0], barnacleEntity->origin[1], barnacleEntity->origin[2] + barcontrol->factor2);
					Vector3GoldSrcToBullet(barnacle_origin);

					rig->barnacle_z_offset = barcontrol->factor3;
					FloatGoldSrcToBullet(&rig->barnacle_z_offset);

					auto transat = MatrixLookAt(rigtrans, barnacle_origin, fwd);

					auto inv = rigtrans.inverse();

					btTransform localrig1;
					localrig1.mult(inv, transat);

					btVector3 offset(barcontrol->offsetX, barcontrol->offsetY, barcontrol->offsetZ);
					Vector3GoldSrcToBullet(offset);
					localrig1.setOrigin(offset);

					rig->barnacle_drag_offset = offset;

					auto constraint = new btSliderConstraint(*rig->rigbody, localrig1, true);

					auto distance = barnacle_origin.distance(rigtrans.getOrigin());

					rig->barnacle_z_init = distance - rig->barnacle_z_offset;
					rig->barnacle_z_final = distance;

					constraint->setLowerAngLimit(M_PI * -1);
					constraint->setUpperAngLimit(M_PI * 1);
					constraint->setLowerLinLimit(0);
					constraint->setUpperLinLimit(rig->barnacle_z_init);
					constraint->setDbgDrawSize(1);

					rig->barnacle_constraint_slider = constraint;

					ragdoll->m_barnacleConstraintArray.emplace_back(constraint);

					m_dynamicsWorld->addConstraint(constraint);

					float factor1 = barcontrol->factor1;
					FloatGoldSrcToBullet(&factor1);
					rig->barnacle_force = factor1;
				}
				else if (barcontrol->type == RAGDOLL_BARNACLE_DOF6)
				{
					if (std::find(ragdoll->m_barnacleDragRigBody.begin(), ragdoll->m_barnacleDragRigBody.end(), rig) == ragdoll->m_barnacleDragRigBody.end())
						ragdoll->m_barnacleDragRigBody.emplace_back(rig);

					btVector3 fwd(1, 0, 0);

					btTransform rigtrans = rig->rigbody->getWorldTransform();

					btVector3 barnacle_origin(barnacleEntity->origin[0], barnacleEntity->origin[1], barnacleEntity->origin[2] + barcontrol->factor2);
					Vector3GoldSrcToBullet(barnacle_origin);

					rig->barnacle_z_offset = barcontrol->factor3;
					FloatGoldSrcToBullet(&rig->barnacle_z_offset);

					auto transat = MatrixLookAt(rigtrans, barnacle_origin, fwd);

					auto inv = rigtrans.inverse();

					btTransform localrig1;
					localrig1.mult(inv, transat);

					btVector3 offset(barcontrol->offsetX, barcontrol->offsetY, barcontrol->offsetZ);
					Vector3GoldSrcToBullet(offset);
					localrig1.setOrigin(offset);

					rig->barnacle_drag_offset = offset;

					auto constraint = new btGeneric6DofConstraint(*rig->rigbody, localrig1, true);

					auto distance = barnacle_origin.distance(rigtrans.getOrigin());

					rig->barnacle_z_init = distance - rig->barnacle_z_offset;
					rig->barnacle_z_final = distance;

					constraint->setAngularLowerLimit(btVector3(M_PI * -1, M_PI * -1, M_PI * -1));
					constraint->setAngularUpperLimit(btVector3(M_PI * 1, M_PI * 1, M_PI * 1));
					constraint->setLinearLowerLimit(btVector3(0, 0, 0));
					constraint->setLinearUpperLimit(btVector3(rig->barnacle_z_init, 0, 0));
					constraint->setDbgDrawSize(1);

					rig->barnacle_constraint_dof6 = constraint;

					ragdoll->m_barnacleConstraintArray.emplace_back(constraint);

					m_dynamicsWorld->addConstraint(constraint);

					float factor1 = barcontrol->factor1;
					FloatGoldSrcToBullet(&factor1);
					rig->barnacle_force = factor1;
				}
				else if (barcontrol->type == RAGDOLL_BARNACLE_CHEWFORCE)
				{
					if (std::find(ragdoll->m_barnacleChewRigBody.begin(), ragdoll->m_barnacleChewRigBody.end(), rig) == ragdoll->m_barnacleChewRigBody.end())
						ragdoll->m_barnacleChewRigBody.emplace_back(rig);

					float factor1 = barcontrol->factor1;
					FloatGoldSrcToBullet(&factor1);
					rig->barnacle_chew_force = factor1;

					rig->barnacle_chew_duration = barcontrol->factor2;
				}
				else if (barcontrol->type == RAGDOLL_BARNACLE_CHEWLIMIT)
				{
					if (std::find(ragdoll->m_barnacleChewRigBody.begin(), ragdoll->m_barnacleChewRigBody.end(), rig) == ragdoll->m_barnacleChewRigBody.end())
						ragdoll->m_barnacleChewRigBody.emplace_back(rig);

					rig->barnacle_chew_duration = barcontrol->factor2;

					float factor3 = barcontrol->factor3;
					FloatGoldSrcToBullet(&factor3);
					rig->barnacle_chew_up_z = factor3;
				}
			}
		}
	}
}

bool CPhysicsManager::UpdateKinematic(CRagdollBody *ragdoll, int iActivityType, entity_state_t *curstate)
{
	if (ragdoll->m_bUpdateKinematic && curstate->msg_time > ragdoll->m_flUpdateKinematicTime)
	{
		ragdoll->m_bUpdateKinematic = false;
		goto update_kinematic;
	}

	if (ragdoll->m_iActivityType == iActivityType)
		return false;

	//Playing death anim or barnacle anim?
	if (ragdoll->m_iActivityType == 0 && iActivityType > 0)
	{
		if (curstate->sequence >= 0 && curstate->sequence < (int)ragdoll->m_animcontrol.size())
		{
			if (curstate->frame < ragdoll->m_animcontrol[curstate->sequence].frame)
			{
				return false;
			}
		}
	}

	ragdoll->m_iActivityType = iActivityType;

update_kinematic:

	for (auto &itor : ragdoll->m_rigbodyMap)
	{
		auto rig = itor.second;

		/*if (rig->flags & RIG_FL_JIGGLE)
		{
			if (!ragdoll->m_iActivityType)
			{
				for (auto &s : m_staticMap)
				{
					auto &staticBody = s.second;

					rig->rigbody->setIgnoreCollisionCheck(staticBody->m_rigbody, true);
				}
			}
			else
			{
				for (auto &s : m_staticMap)
				{
					auto &staticBody = s.second;

					rig->rigbody->setIgnoreCollisionCheck(staticBody->m_rigbody, false);
				}
			}
		}*/

		if (!ragdoll->m_iActivityType && !(rig->flags & RIG_FL_JIGGLE))
		{
			rig->rigbody->setCollisionFlags(rig->oldCollisionFlags | btCollisionObject::CF_KINEMATIC_OBJECT);
			rig->rigbody->setActivationState(DISABLE_DEACTIVATION);
		}
		else
		{
			rig->rigbody->setCollisionFlags(rig->oldCollisionFlags);
			rig->rigbody->forceActivationState(ACTIVE_TAG);
		}
	}

	return true;
}

CRagdollBody *CPhysicsManager::CreateRagdoll(
	ragdoll_config_t *cfg,
	int entindex, 
	bool bIsPlayer)
{
	auto ragdoll = new CRagdollBody();

	mstudiobone_t *pbones = (mstudiobone_t *)((byte *)(*pstudiohdr) + (*pstudiohdr)->boneindex);

	//Save bone relative transform

	for (int i = 0; i < (*pstudiohdr)->numbones; ++i)
	{
		int parent = pbones[i].parent;
		if (parent == -1)
		{
			Matrix3x4ToTransform((*pbonetransform)[i], ragdoll->m_boneRelativeTransform[i]);
		}
		else
		{
			btTransform matrix;

			Matrix3x4ToTransform((*pbonetransform)[i], matrix);

			btTransform parentmatrix;
			Matrix3x4ToTransform((*pbonetransform)[pbones[i].parent], parentmatrix);
			
			auto relative = parentmatrix.inverse() * matrix;

			ragdoll->m_boneRelativeTransform[i] = relative;
		}
	}

	for (size_t i = 0; i < cfg->rigcontrol.size(); ++i)
	{
		auto rigcontrol = &cfg->rigcontrol[i];

		CRigBody *rig = CreateRigBody((*pstudiohdr), rigcontrol);
		if (rig)
		{
			ragdoll->m_keyBones.emplace_back(rigcontrol->boneindex);
			ragdoll->m_rigbodyMap[rigcontrol->name] = rig;

			if (rig->name == "Pelvis")
				ragdoll->m_pelvisRigBody = rig;

			rig->rigbody->setFriction(1);
			rig->rigbody->setRollingFriction(1);
			rig->rigbody->setCcdMotionThreshold(1e-7);
			rig->rigbody->setCcdSweptSphereRadius(0.5);

			rig->oldActivitionState = rig->rigbody->getActivationState();
			rig->oldCollisionFlags = rig->rigbody->getCollisionFlags();
		}

		//rig->buoyancy = new BuoyancyAction(rig);
		//m_dynamicsWorld->addAction(rig->buoyancy);
	}

	for (int i = 0; i < (*pstudiohdr)->numbones; ++i)
	{
		if (std::find(ragdoll->m_keyBones.begin(), ragdoll->m_keyBones.end(), i) == ragdoll->m_keyBones.end())
			ragdoll->m_nonKeyBones.emplace_back(i);
	}

	for (size_t i = 0; i < cfg->cstcontrol.size(); ++i)
	{
		auto cstcontrol = &cfg->cstcontrol[i];

		auto constraint = CreateConstraint(ragdoll, (*pstudiohdr), cstcontrol);

		if (constraint)
		{
			ragdoll->m_constraintArray.emplace_back(constraint);
			m_dynamicsWorld->addConstraint(constraint, true);
		}
	}

	for (size_t i = 0; i < cfg->watercontrol.size(); ++i)
	{
		auto water_control = &cfg->watercontrol[i];

		CreateWaterControl(ragdoll, (*pstudiohdr), water_control);
	}

	ragdoll->m_entindex = entindex;
	ragdoll->m_isPlayer = bIsPlayer;
	ragdoll->m_studiohdr = (*pstudiohdr);
	ragdoll->m_animcontrol = cfg->animcontrol;
	ragdoll->m_barcontrol = cfg->barcontrol;
	ragdoll->m_garcontrol = cfg->garcontrol;

	m_ragdollMap[entindex] = ragdoll;

	return ragdoll;
}

int CPhysicsManager::GetSequenceActivityType(CRagdollBody *ragdoll, entity_state_t* entstate)
{
	if (entstate->sequence >= 0 && entstate->sequence < (int)ragdoll->m_animcontrol.size())
	{
		return ragdoll->m_animcontrol[entstate->sequence].activity;
	}

	return 0;
}
