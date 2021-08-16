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

btScalar G2BScale = 0.2;
btScalar B2GScale = 1 / G2BScale;

extern studiohdr_t **pstudiohdr;
extern model_t **r_model;
extern float(*pbonetransform)[128][3][4];
extern float(*plighttransform)[128][3][4]; 
extern cvar_t *bv_debug;
extern cvar_t *bv_simrate;
extern cvar_t *bv_scale;
extern cvar_t *bv_force_player_ragdoll;
extern model_t *r_worldmodel;
extern int *r_visframecount;

int EngineGetMaxKnownModel(void);
int EngineGetModelIndex(model_t *mod);
model_t *EngineGetModelByIndex(int index);
bool IsEntityPresent(cl_entity_t* ent);
bool IsEntityBarnacle(cl_entity_t* ent);
int GetSequenceActivityType(model_t *mod, entity_state_t* entstate);
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
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_BLEND);
	qglDisable(GL_DEPTH_TEST);

	qglLineWidth(0.5f);

	gEngfuncs.pTriAPI->Color4f(color1.getX(), color1.getY(), color1.getZ(), 1.0f);
	gEngfuncs.pTriAPI->Begin(TRI_LINES);

	vec3_t from = { from1.getX(), from1.getY(), from1.getZ() };
	vec3_t to = { to1.getX(), to1.getY(), to1.getZ() };

	Vec3BulletToGoldSrc(from);
	Vec3BulletToGoldSrc(to);

	gEngfuncs.pTriAPI->Vertex3fv(from);
	gEngfuncs.pTriAPI->Vertex3fv(to);
	gEngfuncs.pTriAPI->End();

	qglEnable(GL_DEPTH_TEST);
	qglEnable(GL_BLEND);
	qglEnable(GL_TEXTURE_2D);
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
}

void CPhysicsManager::GenerateIndexedVertexArray(model_t *mod, indexvertexarray_t *va)
{
	va->iNumFaces = 0;
	va->iCurFace = 0;
	va->iNumVerts = 0;
	va->iCurVert = 0;

	if (r_worldmodel == mod)
	{
		auto surf = r_worldmodel->surfaces;

		for (int i = 0; i < r_worldmodel->numsurfaces; i++)
		{
			if ((surf[i].flags & (SURF_DRAWTURB | SURF_UNDERWATER)))
				continue;

			for (auto poly = surf[i].polys; poly; poly = poly->next)
				va->iNumVerts += 3 + (poly->numverts - 3) * 3;

			va->iNumFaces++;
		}

		if (!va->iNumVerts)
			return;

		if (!va->iNumFaces)
			return;

		va->vVertexBuffer = new brushvertex_t[va->iNumVerts];

		va->vFaceBuffer = new brushface_t[va->iNumFaces];

		for (int i = 0; i < r_worldmodel->numsurfaces; i++)
		{
			if ((surf[i].flags & (SURF_DRAWTURB | SURF_UNDERWATER)))
				continue;

			auto poly = surf[i].polys;

			brushface_t *face = &va->vFaceBuffer[va->iCurFace];

			face->start_vertex = va->iCurVert;
			for (poly = surf[i].polys; poly; poly = poly->next)
			{
				auto v = poly->verts[0];
				brushvertex_t pVertexes[3];

				for (int j = 0; j < 3; j++, v += VERTEXSIZE)
				{
					pVertexes[j].pos[0] = v[0];
					pVertexes[j].pos[1] = v[1];
					pVertexes[j].pos[2] = v[2];
					Vec3GoldSrcToBullet(pVertexes[j].pos);
				}
				memcpy(&va->vVertexBuffer[va->iCurVert], &pVertexes[0], sizeof(brushvertex_t)); va->iCurVert++;
				memcpy(&va->vVertexBuffer[va->iCurVert], &pVertexes[1], sizeof(brushvertex_t)); va->iCurVert++;
				memcpy(&va->vVertexBuffer[va->iCurVert], &pVertexes[2], sizeof(brushvertex_t)); va->iCurVert++;

				for (int j = 0; j < (poly->numverts - 3); j++, v += VERTEXSIZE)
				{
					memcpy(&pVertexes[1], &pVertexes[2], sizeof(brushvertex_t));

					pVertexes[2].pos[0] = v[0];
					pVertexes[2].pos[1] = v[1];
					pVertexes[2].pos[2] = v[2];
					Vec3GoldSrcToBullet(pVertexes[2].pos);

					memcpy(&va->vVertexBuffer[va->iCurVert], &pVertexes[0], sizeof(brushvertex_t)); va->iCurVert++;
					memcpy(&va->vVertexBuffer[va->iCurVert], &pVertexes[1], sizeof(brushvertex_t)); va->iCurVert++;
					memcpy(&va->vVertexBuffer[va->iCurVert], &pVertexes[2], sizeof(brushvertex_t)); va->iCurVert++;
				}
			}

			face->num_vertexes = va->iCurVert - face->start_vertex;
			va->iCurFace++;
		}
	}
	else
	{
		auto psurf = &mod->surfaces[mod->firstmodelsurface];
		for (int i = 0; i < mod->nummodelsurfaces; i++, psurf++)
		{
			if (psurf->flags & (SURF_DRAWTURB | SURF_UNDERWATER))
				continue;

			for (auto poly = psurf->polys; poly; poly = poly->next)
				va->iNumVerts += 3 + (poly->numverts - 3) * 3;

			va->iNumFaces++;
		}

		if (!va->iNumVerts)
			return;

		if (!va->iNumFaces)
			return;

		va->vVertexBuffer = new brushvertex_t[va->iNumVerts];

		va->vFaceBuffer = new brushface_t[va->iNumFaces];

		psurf = &mod->surfaces[mod->firstmodelsurface];
		for (int i = 0; i < mod->nummodelsurfaces; i++, psurf++)
		{
			if (psurf->flags & (SURF_DRAWTURB | SURF_UNDERWATER))
				continue;

			brushface_t *face = &va->vFaceBuffer[va->iCurFace];

			face->start_vertex = va->iCurVert;
			for (auto poly = psurf->polys; poly; poly = poly->next)
			{
				auto v = poly->verts[0];
				brushvertex_t pVertexes[3];

				for (int j = 0; j < 3; j++, v += VERTEXSIZE)
				{
					pVertexes[j].pos[0] = v[0];
					pVertexes[j].pos[1] = v[1];
					pVertexes[j].pos[2] = v[2];
					Vec3GoldSrcToBullet(pVertexes[j].pos);
				}
				memcpy(&va->vVertexBuffer[va->iCurVert], &pVertexes[0], sizeof(brushvertex_t)); va->iCurVert++;
				memcpy(&va->vVertexBuffer[va->iCurVert], &pVertexes[1], sizeof(brushvertex_t)); va->iCurVert++;
				memcpy(&va->vVertexBuffer[va->iCurVert], &pVertexes[2], sizeof(brushvertex_t)); va->iCurVert++;

				for (int j = 0; j < (poly->numverts - 3); j++, v += VERTEXSIZE)
				{
					memcpy(&pVertexes[1], &pVertexes[2], sizeof(brushvertex_t));

					pVertexes[2].pos[0] = v[0];
					pVertexes[2].pos[1] = v[1];
					pVertexes[2].pos[2] = v[2];
					Vec3GoldSrcToBullet(pVertexes[2].pos);

					memcpy(&va->vVertexBuffer[va->iCurVert], &pVertexes[0], sizeof(brushvertex_t)); va->iCurVert++;
					memcpy(&va->vVertexBuffer[va->iCurVert], &pVertexes[1], sizeof(brushvertex_t)); va->iCurVert++;
					memcpy(&va->vVertexBuffer[va->iCurVert], &pVertexes[2], sizeof(brushvertex_t)); va->iCurVert++;
				}
			}

			face->num_vertexes = va->iCurVert - face->start_vertex;
			va->iCurFace++;
		}
	}	
}

void CPhysicsManager::CreateStatic(cl_entity_t *ent, indexvertexarray_t *va)
{
	int iNumTris = 0;
	for (int i = 0; i < va->iNumFaces; i++)
	{
		auto &face = va->vFaceBuffer[i];
		for (int j = 1; j < face.num_vertexes - 1; ++j)
		{
			va->vIndiceBuffer.emplace_back(face.start_vertex);
			va->vIndiceBuffer.emplace_back(face.start_vertex + j);
			va->vIndiceBuffer.emplace_back(face.start_vertex + j + 1);
			iNumTris++;
		}
	}

	if (!iNumTris)
	{
		auto staticbody = new CStaticBody;

		staticbody->m_rigbody = NULL;
		staticbody->m_entindex = ent->index;
		staticbody->m_iva = va;

		m_staticMap[ent->index] = staticbody;
		return;
	}

	const int vertStride = sizeof(float[3]);
	const int indexStride = 3 * sizeof(int);

	auto vertexArray = new btTriangleIndexVertexArray(iNumTris, va->vIndiceBuffer.data(), indexStride,
		va->iNumVerts, (float *)va->vVertexBuffer->pos, vertStride);

	auto meshShape = new btBvhTriangleMeshShape(vertexArray, true, true);

	btDefaultMotionState* motionState = new btDefaultMotionState();

	btRigidBody::btRigidBodyConstructionInfo cInfo(0.0f, motionState, meshShape);

	btRigidBody* body = new btRigidBody(cInfo);

	body->setFriction(1.0f);

	body->setRollingFriction(1.0f);

	float matrix[4][4];
	RotateForEntity(ent, matrix);

	if (0 != memcmp(matrix, r_identity_matrix, sizeof(matrix)))
	{
		float matrix_transposed[4][4];
		Matrix4x4_Transpose(matrix_transposed, matrix);

		btTransform worldtrans;
		worldtrans.setFromOpenGLMatrix((float *)matrix_transposed);

		TransformGoldSrcToBullet(worldtrans);

		body->setWorldTransform(worldtrans);
	}

	m_dynamicsWorld->addRigidBody(body);

	auto staticbody = new CStaticBody;

	staticbody->m_rigbody = body;
	staticbody->m_entindex = ent->index;
	staticbody->m_iva = va;

	m_staticMap[ent->index] = staticbody;
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

	auto iva = new indexvertexarray_t;

	iva->iNumVerts = BARNACLE_SEGMENTS * 8;
	iva->iNumFaces = BARNACLE_SEGMENTS * 2;

	iva->vVertexBuffer = new brushvertex_t[iva->iNumVerts];

	iva->vFaceBuffer = new brushface_t[iva->iNumFaces];

	int iStartVertex = 0;
	int iNumVerts = 0;
	int iNumFace = 0;

	for (int x = 0; x < BARNACLE_SEGMENTS; x++)
	{
		float xSegment = (float)x / (float)BARNACLE_SEGMENTS;
		float xSegment2 = (float)(x + 1) / (float)BARNACLE_SEGMENTS;

		//layer 1

		iva->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * BARNACLE_RADIUS1;
		iva->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * BARNACLE_RADIUS1;
		iva->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT1;

		iNumVerts ++;

		iva->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * BARNACLE_RADIUS2;
		iva->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * BARNACLE_RADIUS2;
		iva->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT2;

		iNumVerts++;

		iva->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS2;
		iva->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS2;
		iva->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT2;

		iNumVerts++;

		iva->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS1;
		iva->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS1;
		iva->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT1;

		iNumVerts++;

		iva->vFaceBuffer[iNumFace].start_vertex = iStartVertex;
		iva->vFaceBuffer[iNumFace].num_vertexes = 4;
		iNumFace++;

		iStartVertex = iNumVerts;

		// layer 2

		iva->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * BARNACLE_RADIUS2;
		iva->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * BARNACLE_RADIUS2;
		iva->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT2;

		iNumVerts++;

		iva->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * BARNACLE_RADIUS3;
		iva->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * BARNACLE_RADIUS3;
		iva->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT3;

		iNumVerts++;

		iva->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS3;
		iva->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS3;
		iva->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT3;

		iNumVerts++;

		iva->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS2;
		iva->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS2;
		iva->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT2;

		iNumVerts++;

		iva->vFaceBuffer[iNumFace].start_vertex = iStartVertex;
		iva->vFaceBuffer[iNumFace].num_vertexes = 4;
		iNumFace++;

		iStartVertex = iNumVerts;
	}

	CreateStatic(ent, iva);
}

void CPhysicsManager::CreateBrushModel(cl_entity_t *ent)
{
	auto itor = m_staticMap.find(ent->index);

	if (itor != m_staticMap.end())
	{
		auto staticBody = itor->second;

		if (ent->index > 0 && staticBody->m_rigbody)
		{
			float matrix[4][4];
			RotateForEntity(ent, matrix);
			
			if (0 != memcmp(matrix, r_identity_matrix, sizeof(matrix)))
			{
				float matrix_transposed[4][4];
				Matrix4x4_Transpose(matrix_transposed, matrix);

				btTransform worldtrans;
				worldtrans.setFromOpenGLMatrix((float *)matrix_transposed);

				TransformGoldSrcToBullet(worldtrans);

				staticBody->m_rigbody->setWorldTransform(worldtrans);
			}
		}

		return;
	}

	auto iva = new indexvertexarray_t;

	GenerateIndexedVertexArray(ent->model, iva);

	CreateStatic(ent, iva);
}

void CPhysicsManager::NewMap(void)
{
	G2BScale = bv_scale->value;
	B2GScale = 1 / bv_scale->value;

	ReloadConfig();
	RemoveAllRagdolls();
	RemoveAllStatics();

	auto r_worldentity = gEngfuncs.GetEntityByIndex(0);

	r_worldmodel = r_worldentity->model;

	CreateBrushModel(r_worldentity);
}

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
}

void CPhysicsManager::DebugDraw(void)
{
	if (bv_debug->value)
	{
		for (auto &p : m_staticMap)
		{
			auto &staticbody = p.second;

			if (staticbody->m_rigbody)
			{
				if (bv_debug->value == 2 && staticbody->m_entindex)
				{
					btVector3 color(0, 0.75, 0.75f);
					m_dynamicsWorld->debugDrawObject(staticbody->m_rigbody->getWorldTransform(), staticbody->m_rigbody->getCollisionShape(), color);
				}
				else if (bv_debug->value == 3)
				{
					btVector3 color(0.25, 0.25, 0.25f);
					m_dynamicsWorld->debugDrawObject(staticbody->m_rigbody->getWorldTransform(), staticbody->m_rigbody->getCollisionShape(), color);
				}
			}
		}

		for (auto &p : m_ragdollMap)
		{
			auto ragdoll = p.second;

			auto &rigmap = ragdoll->m_rigbodyMap;
			for (auto &rig : rigmap)
			{
				auto rigbody = rig.second->rigbody;

				btVector3 color(1, 1, 1);
				m_dynamicsWorld->debugDrawObject(rigbody->getWorldTransform(), rigbody->getCollisionShape(), color);
			}

			auto &cstarray = ragdoll->m_constraintArray;

			for (auto p : cstarray)
			{
				m_dynamicsWorld->debugDrawConstraint(p);
			}

			auto &cstarray2 = ragdoll->m_barnacleConstraintArray;

			for (auto p : cstarray2)
			{
				m_dynamicsWorld->debugDrawConstraint(p);
			}
		}
	}
}

void CPhysicsManager::ReleaseRagdollFromBarnacle(CRagdoll *ragdoll)
{
	ragdoll->m_barnacleindex = -1;
	ragdoll->m_barnacleDragRigBody.clear();
	ragdoll->m_barnacleChewRigBody.clear();
	for (auto cst : ragdoll->m_barnacleConstraintArray)
	{
		m_dynamicsWorld->removeConstraint(cst);
		delete cst;
	}
	for (auto rig : ragdoll->m_barnacleDragRigBody)
	{
		rig->barnacle_constraint_slider = NULL;
		rig->barnacle_constraint_dof6 = NULL;
	}
	ragdoll->m_barnacleConstraintArray.clear();
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

bool CPhysicsManager::GetRagdollOrigin(CRagdoll *ragdoll, float *origin)
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

bool CPhysicsManager::UpdateRagdoll(cl_entity_t *ent, CRagdoll *ragdoll, double frame_time, double client_time)
{
	if (ragdoll->m_barnacleindex != -1)
	{
		bool bDraging = true;

		if (GetSequenceActivityType(ent->model, &ent->curstate) != 2)
		{
			ReleaseRagdollFromBarnacle(ragdoll);
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

	return true;
}

void CPhysicsManager::UpdateTempEntity(TEMPENTITY **ppTempEntActive, double frame_time, double client_time)
{
	for (auto itor = m_ragdollMap.begin(); itor != m_ragdollMap.end();)
	{
		auto pRagdoll = itor->second;

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
	m_dynamicsWorld->stepSimulation(frametime, 16, 1.0f / bv_simrate->value);
}

void CPhysicsManager::SetGravity(float velocity)
{
	float goldsrc_velocity = -velocity;

	FloatGoldSrcToBullet(&goldsrc_velocity);

	m_dynamicsWorld->setGravity(btVector3(0, 0, goldsrc_velocity));
}

#define NL_PRESENT 0
#define NL_NEEDS_LOADED 1
#define NL_UNREFERENCED 2
#define NL_CLIENT 3

void CPhysicsManager::ReloadConfig(void)
{
	int maxNum = EngineGetMaxKnownModel();

	if (m_ragdoll_config.size() < maxNum)
		m_ragdoll_config.resize(maxNum);

	for (int i = 0; i < maxNum; ++i)
	{
		if (m_ragdoll_config[i])
		{
			delete m_ragdoll_config[i];
			m_ragdoll_config[i] = NULL;
		}
	}

	for (int i = 0; i < *mod_numknown; ++i)
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
		Sys_ErrorEx("LoadRagdollConfig: Invalid model index\n");
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
		Sys_ErrorEx("LoadRagdollConfig: Invalid name %s\n", fullname.c_str());
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
			iParsingState = 0;
			continue;
		}
		else if (!strcmp(text, "[RigidBody]"))
		{
			iParsingState = 1;
			continue;
		}
		else if (!strcmp(text, "[Constraint]"))
		{
			iParsingState = 2;
			continue;
		}
		else if (!strcmp(text, "[Barnacle]"))
		{
			iParsingState = 3;
			continue;
		}

		std::string subname = text;

		if (iParsingState == 0)
		{
			int i_sequence = atoi(subname.c_str());

			if (i_sequence < 0)
				break;

			ptext = gEngfuncs.COM_ParseFile(ptext, text);
			if (!ptext)
				break;

			float f_frame = atof(text);

			cfg->animcontrol[i_sequence] = f_frame;
		}
		else if (iParsingState == 1)
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
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse rigidbody siz for %s\n", name.c_str());
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

			cfg->rigcontrol.emplace_back(subname, i_boneindex, i_pboneindex, i_shape, f_offset, f_size, f_size2, f_mass);
		}
		else if (iParsingState == 2)
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
		else if (iParsingState == 3)
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
	if (rigcontrol->boneindex >= studiohdr->numbones)
	{
		gEngfuncs.Con_Printf("CreateRigBody: Failed to create rigbody for bone %s, boneindex too large (%d >= %d)\n", rigcontrol->name.c_str(), rigcontrol->boneindex, studiohdr->numbones);
		return NULL;
	}

	if (rigcontrol->pboneindex >= studiohdr->numbones)
	{
		gEngfuncs.Con_Printf("CreateRigBody: Failed to create rigbody for bone %s, pboneindex too large (%d >= %d)\n", rigcontrol->name.c_str(), rigcontrol->pboneindex, studiohdr->numbones);
		return NULL;
	}

	mstudiobone_t *pbones = (mstudiobone_t *)((byte *)(*pstudiohdr) + (*pstudiohdr)->boneindex);

	btTransform bonematrix, offsetmatrix;
	Matrix3x4ToTransform((*pbonetransform)[rigcontrol->boneindex], bonematrix);
	TransformGoldSrcToBullet(bonematrix);

	auto boneorigin = bonematrix.getOrigin();
	
	btVector3 pboneorigin((*pbonetransform)[rigcontrol->pboneindex][0][3], (*pbonetransform)[rigcontrol->pboneindex][1][3], (*pbonetransform)[rigcontrol->pboneindex][2][3]);
	Vector3GoldSrcToBullet(pboneorigin);

	btVector3 dir = pboneorigin - boneorigin;
	dir = dir.normalize();

	float offset = rigcontrol->offset;
	FloatGoldSrcToBullet(&offset);

	auto origin = bonematrix.getOrigin();

	origin = origin + dir * offset;

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
		rig->origin = origin;
		rig->dir = dir;
		rig->boneindex = rigcontrol->boneindex;

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
		rig->origin = origin;
		rig->dir = dir;
		rig->boneindex = rigcontrol->boneindex;

		return rig;
	}

	gEngfuncs.Con_Printf("CreateRigBody: Failed to create rigbody %s, invalid shape type %d\n", rigcontrol->name.c_str(), rigcontrol->shape);
	return NULL;
}

btTypedConstraint *CPhysicsManager::CreateConstraint(CRagdoll *ragdoll, studiohdr_t *studiohdr, ragdoll_cst_control_t *cstcontrol)
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
		gEngfuncs.Con_Printf("CreateRigBody: Failed to create rigbody for bone %s, boneindex1 too large (%d >= %d)\n", cstcontrol->name.c_str(), cstcontrol->boneindex1, studiohdr->numbones);
		return NULL;
	}

	if (cstcontrol->boneindex2 >= studiohdr->numbones)
	{
		gEngfuncs.Con_Printf("CreateRigBody: Failed to create rigbody for bone %s, boneindex2 too large (%d >= %d)\n", cstcontrol->name.c_str(), cstcontrol->boneindex2, studiohdr->numbones);
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
			cst->setDbgDrawSize(1);
		}
		else
		{
			cst->setDbgDrawSize(0.25f);
		}
		
		cst->setLimit(cstcontrol->factor1 * M_PI, cstcontrol->factor2 * M_PI, cstcontrol->factor3 * M_PI);

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
			cst->setDbgDrawSize(1);
		}
		else
		{
			cst->setDbgDrawSize(0.25f);
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
			if (staticBody->m_iva)
			{
				delete[]staticBody->m_iva->vFaceBuffer;
				delete[]staticBody->m_iva->vVertexBuffer;
				delete staticBody->m_iva;
			}

			delete staticBody; 
		}
	}

	m_staticMap.clear();
}

void CPhysicsManager::RemoveAllRagdolls()
{
	for (auto rag : m_ragdollMap)
	{
		auto ragdoll = rag.second;

		for (auto p : ragdoll->m_constraintArray)
		{
			m_dynamicsWorld->removeConstraint(p);
			delete p;
		}

		ragdoll->m_constraintArray.clear();

		for (auto p : ragdoll->m_barnacleConstraintArray)
		{
			m_dynamicsWorld->removeConstraint(p);
			delete p;
		}

		for (auto p : ragdoll->m_rigbodyMap)
		{
			m_dynamicsWorld->removeRigidBody(p.second->rigbody);
			delete p.second->rigbody;
			delete p.second;
		}

		ragdoll->m_rigbodyMap.clear();

		delete ragdoll;
	}
	m_ragdollMap.clear();
}

ragdoll_itor CPhysicsManager::FreeRagdollInternal(ragdoll_itor &itor)
{
	auto ragdoll = itor->second;

	RagdollDestroyCallback(ragdoll->m_entindex);

	for (auto p : ragdoll->m_constraintArray)
	{
		m_dynamicsWorld->removeConstraint(p);
		delete p;
	}

	ragdoll->m_constraintArray.clear();

	for (auto p : ragdoll->m_rigbodyMap)
	{
		m_dynamicsWorld->removeRigidBody(p.second->rigbody);
		delete p.second;
	}

	ragdoll->m_rigbodyMap.clear();

	delete ragdoll;

	return m_ragdollMap.erase(itor);
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
		//gEngfuncs.Con_Printf("MergeBarnacleBones: not found\n");
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

	/*for (int i = 0; i < hdr->numbones; ++i)
	{
		gEngfuncs.Con_Printf("SetupBarnacleBones: pbones[%d] = %s, parent = %d\n", i, pbones[i].name, pbones[i].parent);
	}*/
}

bool CPhysicsManager::SetupBones(studiohdr_t *hdr, int entindex)
{
	auto itor = m_ragdollMap.find(entindex);

	if (itor == m_ragdollMap.end())
	{
		//gEngfuncs.Con_Printf("SetupPlayerBones: not found\n");
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

CRagdoll *CPhysicsManager::FindRagdoll(int tentindex)
{
	auto itor = FindRagdollEx(tentindex);

	if (itor != m_ragdollMap.end())
	{
		return itor->second;
	}

	return NULL;
}

bool CPhysicsManager::CreateRagdoll(
	ragdoll_config_t *cfg,
	int entindex, 
	studiohdr_t *studiohdr,
	int iActivityType,
	float *origin, 
	float *velocity, 
	cl_entity_t *barnacle,
	bool isplayer)
{
	if(FindRagdoll(entindex))
	{
		gEngfuncs.Con_Printf("CreateRagdoll: Already exists\n");
		return true;
	}

	btVector3 player_origin(origin[0], origin[1], origin[2]);
	Vector3GoldSrcToBullet(player_origin);

	auto ragdoll = new CRagdoll();

	mstudiobone_t *pbones = (mstudiobone_t *)((byte *)(*pstudiohdr) + (*pstudiohdr)->boneindex);

	//Save bone relative transform

	for (int i = 0; i < studiohdr->numbones; ++i)
	{
		if (bv_debug->value)
		{
			gEngfuncs.Con_Printf("CreateRagdoll: pbones[%d] = %s, parent = %d\n", i, pbones[i].name, pbones[i].parent);
		}

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

		CRigBody *rig = CreateRigBody(studiohdr, rigcontrol);
		if (rig)
		{
			ragdoll->m_keyBones.emplace_back(rigcontrol->boneindex);
			ragdoll->m_rigbodyMap[rigcontrol->name] = rig;

			if (rig->name == "Pelvis")
				ragdoll->m_pelvisRigBody = rig;
			else if (rig->name == "Head")
				ragdoll->m_headRigBody = rig;

			btVector3 vel(velocity[0], velocity[1], velocity[2]);

			if (vel.length() > 500)
				vel = vel.normalize() * 500;

			Vector3GoldSrcToBullet(vel);

			rig->rigbody->setLinearVelocity(vel);

			rig->rigbody->setFriction(0.5f);

			rig->rigbody->setRollingFriction(0.5f);

			m_dynamicsWorld->addRigidBody(rig->rigbody);

			if (iActivityType == 2 && barnacle)
			{
				ragdoll->m_barnacleindex = barnacle->index;

				for (size_t j = 0; j < cfg->barcontrol.size(); ++j)
				{
					auto barcontrol = &cfg->barcontrol[j];

					if (barcontrol->name == rig->name)
					{
						if (barcontrol->type == RAGDOLL_BARNACLE_SLIDER)
						{
							if (std::find(ragdoll->m_barnacleDragRigBody.begin(), ragdoll->m_barnacleDragRigBody.end(), rig) == ragdoll->m_barnacleDragRigBody.end())
								ragdoll->m_barnacleDragRigBody.emplace_back(rig);

							btVector3 fwd(1, 0, 0);

							btTransform rigtrans = rig->rigbody->getWorldTransform();
						
							btVector3 barnacle_origin(barnacle->origin[0], barnacle->origin[1], barnacle->origin[2] + barcontrol->factor2);
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

							rig->barnacle_force = barcontrol->factor1;

							FloatGoldSrcToBullet(&rig->barnacle_force);

							rig->barnacle_constraint_slider = constraint;

							ragdoll->m_barnacleConstraintArray.emplace_back(constraint);
							m_dynamicsWorld->addConstraint(constraint);
							break;
						}
						else if (barcontrol->type == RAGDOLL_BARNACLE_DOF6)
						{
							if (std::find(ragdoll->m_barnacleDragRigBody.begin(), ragdoll->m_barnacleDragRigBody.end(), rig) == ragdoll->m_barnacleDragRigBody.end())
								ragdoll->m_barnacleDragRigBody.emplace_back(rig);

							btVector3 fwd(1, 0, 0);

							btTransform rigtrans = rig->rigbody->getWorldTransform();

							btVector3 barnacle_origin(barnacle->origin[0], barnacle->origin[1], barnacle->origin[2] + barcontrol->factor2);
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

							rig->barnacle_force = barcontrol->factor1;
							FloatGoldSrcToBullet(&rig->barnacle_force);

							rig->barnacle_constraint_dof6 = constraint;

							ragdoll->m_barnacleConstraintArray.emplace_back(constraint);
							m_dynamicsWorld->addConstraint(constraint);
						}
						else if (barcontrol->type == RAGDOLL_BARNACLE_CHEWFORCE)
						{
							if (std::find(ragdoll->m_barnacleChewRigBody.begin(), ragdoll->m_barnacleChewRigBody.end(), rig) == ragdoll->m_barnacleChewRigBody.end())
								ragdoll->m_barnacleChewRigBody.emplace_back(rig);

							rig->barnacle_chew_force = barcontrol->factor1;
							FloatGoldSrcToBullet(&rig->barnacle_chew_force);

							rig->barnacle_chew_duration = barcontrol->factor2;
						}
						else if (barcontrol->type == RAGDOLL_BARNACLE_CHEWLIMIT)
						{
							if (std::find(ragdoll->m_barnacleChewRigBody.begin(), ragdoll->m_barnacleChewRigBody.end(), rig) == ragdoll->m_barnacleChewRigBody.end())
								ragdoll->m_barnacleChewRigBody.emplace_back(rig);

							rig->barnacle_chew_duration = barcontrol->factor2;

							rig->barnacle_chew_up_z = barcontrol->factor3;
							FloatGoldSrcToBullet(&rig->barnacle_chew_up_z);
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < studiohdr->numbones; ++i)
	{
		if (std::find(ragdoll->m_keyBones.begin(), ragdoll->m_keyBones.end(), i) == ragdoll->m_keyBones.end())
			ragdoll->m_nonKeyBones.emplace_back(i);
	}

	for (size_t i = 0; i < cfg->cstcontrol.size(); ++i)
	{
		auto cstcontrol = &cfg->cstcontrol[i];

		auto constraint = CreateConstraint(ragdoll, studiohdr, cstcontrol);

		if (constraint)
		{
			ragdoll->m_constraintArray.emplace_back(constraint);
			m_dynamicsWorld->addConstraint(constraint, true);
		}
	}

	ragdoll->m_entindex = entindex;
	ragdoll->m_isPlayer = isplayer;
	ragdoll->m_studiohdr = studiohdr;
	m_ragdollMap[entindex] = ragdoll;

	return true;
}