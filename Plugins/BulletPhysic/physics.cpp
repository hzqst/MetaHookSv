#include "exportfuncs.h"
#include "triangleapi.h"
#include "enginedef.h"
#include "studio.h"
#include "cvardef.h"
#include "physics.h"
#include "qgl.h"
#include "mathlib.h"

const float G2BScale = 0.02540f;
const float B2GScale = 1 / 0.02540f;
const int ValuesX = 0;
const int ValuesY = 2;
const int ValuesZ = 1;

extern studiohdr_t **pstudiohdr;
extern model_t **r_model;
extern float(*pbonetransform)[128][3][4];
extern float(*plighttransform)[128][3][4]; 
extern cvar_t *bv_debug;
extern cvar_t *bv_simrate;
extern model_t *r_worldmodel;

const float r_identity_matrix[4][4] = {
	{1.0f, 0.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 0.0f, 1.0f}
};

void CPhysicsDebugDraw::drawLine(const btVector3& from1, const btVector3& to1, const btVector3& color1)
{
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_BLEND);
	qglDisable(GL_DEPTH_TEST);

	qglLineWidth(0.5f);

	gEngfuncs.pTriAPI->Color4f(color1.getX(), color1.getY(), color1.getZ(), 1.0f);
	gEngfuncs.pTriAPI->Begin(TRI_LINES);

	float from[3] = { from1.getX(), from1.getY(), from1.getZ() };
	float to[3] = { to1.getX(), to1.getY(), to1.getZ() };
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
	 m_worldmodel_iva = NULL;
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

		va->vVertexBuffer = new brushvertex_t[va->iNumVerts];

		va->vFaceBuffer = new brushface_t[va->iNumFaces];

		for (int i = 0; i < r_worldmodel->numsurfaces; i++)
		{
			if ((surf[i].flags & (SURF_DRAWTURB | SURF_UNDERWATER)))
				continue;

			auto poly = surf[i].polys;

			brushface_t *face = &va->vFaceBuffer[va->iCurFace];
			face->index = i;

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

		va->vVertexBuffer = new brushvertex_t[va->iNumVerts];

		va->vFaceBuffer = new brushface_t[va->iNumFaces];

		psurf = &mod->surfaces[mod->firstmodelsurface];
		for (int i = 0; i < mod->nummodelsurfaces; i++, psurf++)
		{
			if (psurf->flags & (SURF_DRAWTURB | SURF_UNDERWATER))
				continue;

			brushface_t *face = &va->vFaceBuffer[va->iCurFace];
			face->index = i;

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

void CPhysicsManager::CreateStatic(int entindex, indexvertexarray_t *va)
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

	const int vertStride = sizeof(float[3]);
	const int indexStride = 3 * sizeof(int);

	auto vertexArray = new btTriangleIndexVertexArray(iNumTris, va->vIndiceBuffer.data(), indexStride,
		va->iNumVerts, (float *)va->vVertexBuffer->pos, vertStride);

	auto meshShape = new btBvhTriangleMeshShape(vertexArray, true, true);

	btDefaultMotionState* motionState = new btDefaultMotionState();

	btRigidBody::btRigidBodyConstructionInfo cInfo(0.0f, motionState, meshShape);

	btRigidBody* body = new btRigidBody(cInfo);

	body->setUserIndex(entindex);

	body->setFriction(1.0f);

	body->setRollingFriction(1.0f);

	m_dynamicsWorld->addRigidBody(body);

	auto staticbody = new CStaticBody;

	staticbody->m_rigbody = body;
	staticbody->m_entindex = entindex;

	m_staticMap[entindex] = staticbody;
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

void CPhysicsManager::CreateForBrushModel(cl_entity_t *ent)
{
	auto itor = m_staticMap.find(ent->index);

	if (itor != m_staticMap.end())
	{
		if (ent->index > 0)
		{
			float matrix[4][4];
			RotateForEntity(ent, matrix);
			
			if (0 != memcmp(matrix, r_identity_matrix, sizeof(matrix)))
			{
				auto staticbody = itor->second;

				float matrix_transposed[4][4];
				Matrix4x4_Transpose(matrix_transposed, matrix);

				btTransform worldtrans;
				worldtrans.setFromOpenGLMatrix((float *)matrix_transposed);

				staticbody->m_rigbody->setWorldTransform(worldtrans);
			}
		}

		return;
	}

	auto iva = new indexvertexarray_t;

	GenerateIndexedVertexArray(ent->model, iva);

	CreateStatic(ent->index, iva);
}

void CPhysicsManager::NewMap(void)
{
	RemoveAllRagdolls();
	RemoveAllStatics();
	RemoveIndexedVertexArray();

	auto r_worldentity = gEngfuncs.GetEntityByIndex(0);

	r_worldmodel = r_worldentity->model;

	CreateForBrushModel(r_worldentity);
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
	for (auto &p : m_ragdollMap)
	{
		auto &rigmap = p.second->m_rigbodyMap;
		for (auto &r : rigmap)
		{
			btVector3 color(1, 1, 1);
			m_dynamicsWorld->debugDrawObject(r.second.rigbody->getWorldTransform(), r.second.rigbody->getCollisionShape(), color);
		}
		auto &cstarray = p.second->m_constraintArray;

		for (auto p : cstarray)
		{
			m_dynamicsWorld->debugDrawConstraint(p);
		}
	}
}

void CPhysicsManager::StepSimulation(double frametime)
{
	if (bv_simrate->value < 16)
		bv_simrate->value = 16;
	else if (bv_simrate->value > 128)
		bv_simrate->value = 128;

	m_dynamicsWorld->stepSimulation(frametime, 16, 1.0f / bv_simrate->value);
}

void CPhysicsManager::SetGravity(float velocity)
{
	m_dynamicsWorld->setGravity(btVector3(0, 0, -velocity));
}

void CPhysicsManager::ReloadConfig(void)
{
	for (auto p : m_ragdoll_config)
	{
		if(p.second)
			delete p.second;
	}
	m_ragdoll_config.clear();
}

ragdoll_config_t *CPhysicsManager::LoadRagdollConfig(const std::string &modelname)
{
	auto itor = m_ragdoll_config.find(modelname);
	if (itor != m_ragdoll_config.end())
	{
		return itor->second;
	}

	std::string name = modelname;
	name = name.substr(0, name.length() - 4);
	name += "_ragdoll.txt";

	char *pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
	if (!pfile)
	{
		m_ragdoll_config[modelname] = NULL;
		gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to load %s\n", name.c_str());
		return NULL;
	}

	auto cfg = new ragdoll_config_t;

	int iParsingState = -1;

	char *ptext = pfile;
	while (1)
	{
		char subname[256] = { 0 };

		ptext = gEngfuncs.COM_ParseFile(ptext, subname);

		if (!ptext)
			break;

		if (!strcmp(subname, "[DeathAnim]"))
		{
			iParsingState = 0;
			continue;
		}
		else if (!strcmp(subname, "[RigidBody]"))
		{
			iParsingState = 1;
			continue;
		}
		else if (!strcmp(subname, "[Constraint]"))
		{
			iParsingState = 2;
			continue;
		}

		if (iParsingState == 0)
		{
			char frame[16] = { 0 };

			ptext = gEngfuncs.COM_ParseFile(ptext, frame);
			if (!ptext)
				break;

			float i_sequence = atof(subname);
			float f_frame = atof(frame);

			cfg->animcontrol[i_sequence] = f_frame;
		}
		else if (iParsingState == 1)
		{
			char boneindex[16] = { 0 };
			char pboneindex[16] = { 0 };
			char shape[16] = { 0 };
			char offset[16] = { 0 };
			char size[16] = { 0 };
			char size2[16] = { 0 };
			
			ptext = gEngfuncs.COM_ParseFile(ptext, boneindex);
			if (!ptext)
				break;

			int i_boneindex = atoi(boneindex);

			ptext = gEngfuncs.COM_ParseFile(ptext, pboneindex);
			if (!ptext)
				break;

			int i_pboneindex = atoi(pboneindex);

			ptext = gEngfuncs.COM_ParseFile(ptext, shape);
			if (!ptext)
				break;

			int i_shape = -1;

			if (!strcmp(shape, "sphere"))
			{
				i_shape = RAGDOLL_SHAPE_SPHERE;
			}
			else if (!strcmp(shape, "capsule"))
			{
				i_shape = RAGDOLL_SHAPE_CAPSULE;
			}
			else
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse shape name %s for %s\n", shape, name.c_str());
				break;
			}

			ptext = gEngfuncs.COM_ParseFile(ptext, offset);
			if (!ptext)
				break;

			float f_offset = atof(offset);

			ptext = gEngfuncs.COM_ParseFile(ptext, size);
			if (!ptext)
				break;

			float f_size = atof(size);

			ptext = gEngfuncs.COM_ParseFile(ptext, size2);
			if (!ptext)
				break;

			float f_size2 = atof(size2);

			cfg->rigcontrol.emplace_back(subname, i_boneindex, i_pboneindex, i_shape, f_offset, f_size, f_size2);
		}
		else if (iParsingState == 2)
		{
			char linktarget[64] = { 0 };
			char type[16] = { 0 };
			char offset1[16] = { 0 };
			char offset2[16] = { 0 };
			char factor1[16] = { 0 };
			char factor2[16] = { 0 };
			char factor3[16] = { 0 };

			ptext = gEngfuncs.COM_ParseFile(ptext, linktarget);
			if (!ptext)
				break;

			ptext = gEngfuncs.COM_ParseFile(ptext, type);
			if (!ptext)
				break;

			int i_type = -1;

			if (!strcmp(type, "conetwist"))
			{
				i_type = RAGDOLL_CONSTRAINT_CONETWIST;
			}
			else if (!strcmp(type, "hinge"))
			{
				i_type = RAGDOLL_CONSTRAINT_HINGE;
			}
			else if (!strcmp(type, "point"))
			{
				i_type = RAGDOLL_CONSTRAINT_POINT;
			}
			else
			{
				gEngfuncs.Con_Printf("LoadRagdollConfig: Failed to parse constraint type %s for %s\n", type, name.c_str());
				break;
			}

			ptext = gEngfuncs.COM_ParseFile(ptext, offset1);
			if (!ptext)
				break;

			float f_offset1 = atof(offset1);

			ptext = gEngfuncs.COM_ParseFile(ptext, offset2);
			if (!ptext)
				break;

			float f_offset2 = atof(offset2);

			ptext = gEngfuncs.COM_ParseFile(ptext, factor1);
			if (!ptext)
				break;

			float f_factor1 = atof(factor1);

			ptext = gEngfuncs.COM_ParseFile(ptext, factor2);
			if (!ptext)
				break;

			float f_factor2 = atof(factor2);

			ptext = gEngfuncs.COM_ParseFile(ptext, factor3);
			if (!ptext)
				break;

			float f_factor3 = atof(factor3);

			cfg->cstcontrol.emplace_back(subname, linktarget, i_type, f_offset1, f_offset2, f_factor1, f_factor2, f_factor3);
		}
	}

	gEngfuncs.COM_FreeFile(pfile);

	m_ragdoll_config[modelname] = cfg;

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

void Matrix3x4ToTransform(const float matrix3x4[3][4] , btTransform &trans)
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

btTypedConstraint *CPhysicsManager::CreateConstraint(CRagdoll *ragdoll, studiohdr_t *hdr, ragdoll_cst_control_t *cstcontrol)
{
	auto itor = ragdoll->m_rigbodyMap.find(cstcontrol->name);
	if (itor == ragdoll->m_rigbodyMap.end())
	{
		gEngfuncs.Con_Printf("CreateConstraint: Failed to create constraint, rig %s not found\n", cstcontrol->name.c_str());
		return NULL;
	}
	auto itor2 = ragdoll->m_rigbodyMap.find(cstcontrol->linktarget);
	if (itor2 == ragdoll->m_rigbodyMap.end())
	{
		gEngfuncs.Con_Printf("CreateConstraint: Failed to create constraint, linked rig %s not found\n", cstcontrol->name.c_str());
		return NULL;
	}

	auto rig1 = itor->second;
	auto rig2 = itor2->second;

	if (cstcontrol->type == RAGDOLL_CONSTRAINT_CONETWIST)
	{
		auto origin1 = rig1.origin + rig1.dir * cstcontrol->offset1;
		auto origin2 = rig2.origin + rig2.dir * cstcontrol->offset2;

		btTransform matrix1;
		matrix1.setIdentity();
		matrix1.setOrigin(origin1);

		btVector3 fwd(1, 0, 0);
		auto jointtrans = MatrixLookAt(matrix1, origin2, fwd);

		auto inv1 = rig1.rigbody->getWorldTransform().inverse();
		auto inv2 = rig2.rigbody->getWorldTransform().inverse();

		btTransform localrig1;
		localrig1.mult(inv1, jointtrans);

		btTransform localrig2;
		localrig2.mult(inv2, jointtrans);

		auto cst = new btConeTwistConstraint(*rig1.rigbody, *rig2.rigbody, localrig1, localrig2);
		cst->setDbgDrawSize(20);
		cst->setLimit(cstcontrol->factor1 * M_PI, cstcontrol->factor2 * M_PI, cstcontrol->factor3 * M_PI);

		return cst;
	}
	else if (cstcontrol->type == RAGDOLL_CONSTRAINT_HINGE)
	{
		auto origin1 = rig1.origin + rig1.dir * cstcontrol->offset1;
		auto origin2 = rig2.origin + rig2.dir * cstcontrol->offset2;

		btTransform matrix1;
		matrix1.setIdentity();
		matrix1.setOrigin(origin1);

		btVector3 fwd(0, 0, 1);
		auto jointtrans = MatrixLookAt(matrix1, origin2, fwd);

		auto inv1 = rig1.rigbody->getWorldTransform().inverse();
		auto inv2 = rig2.rigbody->getWorldTransform().inverse();

		btTransform localrig1;
		localrig1.mult(inv1, jointtrans);

		btTransform localrig2;
		localrig2.mult(inv2, jointtrans);

		auto cst = new btHingeConstraint(*rig1.rigbody, *rig2.rigbody, localrig1, localrig2);
		cst->setAxis(fwd);
		cst->setDbgDrawSize(20);
		cst->setLimit(cstcontrol->factor1 * M_PI, cstcontrol->factor2 * M_PI);

		return cst;
	}
	else if (cstcontrol->type == RAGDOLL_CONSTRAINT_POINT)
	{
		auto origin1 = rig1.origin + rig1.dir * cstcontrol->offset1;
		auto origin2 = rig2.origin + rig2.dir * cstcontrol->offset2;

		btVector3 local1 = rig1.rigbody->getWorldTransform().inverse() * origin1;
		btVector3 local2 = rig1.rigbody->getWorldTransform().inverse() * origin2;

		auto cst = new btPoint2PointConstraint(*rig1.rigbody, *rig2.rigbody, local1, local2);		
		cst->setDbgDrawSize(20);
		return cst;
	}

	gEngfuncs.Con_Printf("CreateConstraint: Failed to create constraint for %s, invalid type %d\n", cstcontrol->name.c_str(), cstcontrol->type);
	return NULL;
}

bool CPhysicsManager::CreateRigBody(studiohdr_t *studiohdr, ragdoll_rig_control_t *rigcontrol, CRigBody &rig)
{
	if (rigcontrol->boneindex >= studiohdr->numbones)
	{
		gEngfuncs.Con_Printf("CreateRigBody: Failed to create rigbody for bone %s, boneindex too large (%d >= %d)\n", rigcontrol->name.c_str(), rigcontrol->boneindex, studiohdr->numbones);
		return false;
	}

	if (rigcontrol->pboneindex >= studiohdr->numbones)
	{
		gEngfuncs.Con_Printf("CreateRigBody: Failed to create rigbody for bone %s, pboneindex too large (%d >= %d)\n", rigcontrol->name.c_str(), rigcontrol->pboneindex, studiohdr->numbones);
		return false;
	}

	mstudiobone_t *pbones = (mstudiobone_t *)((byte *)(*pstudiohdr) + (*pstudiohdr)->boneindex);

	btTransform bonematrix, offsetmatrix;
	Matrix3x4ToTransform((*pbonetransform)[rigcontrol->boneindex], bonematrix);
	
	auto boneorigin = bonematrix.getOrigin();
	
	btVector3 pboneorigin;
	pboneorigin.setX((*pbonetransform)[rigcontrol->pboneindex][0][3]);
	pboneorigin.setY((*pbonetransform)[rigcontrol->pboneindex][1][3]);
	pboneorigin.setZ((*pbonetransform)[rigcontrol->pboneindex][2][3]);

	btVector3 dir = pboneorigin - boneorigin;
	dir = dir.normalize();

	btTransform rigidtransform;

	auto origin = bonematrix.getOrigin();
	origin = origin + dir * rigcontrol->offset;	

	if (rigcontrol->shape == RAGDOLL_SHAPE_SPHERE)
	{
		rigidtransform.setIdentity();
		rigidtransform.setOrigin(origin);
		offsetmatrix.mult(bonematrix.inverse(), rigidtransform);

		auto shape = new btSphereShape(rigcontrol->size);

		BoneMotionState* motionState = new BoneMotionState(bonematrix, offsetmatrix);
		
		float mass = 1;
		btVector3 localInertia;
		shape->calculateLocalInertia(mass, localInertia);

		btRigidBody::btRigidBodyConstructionInfo cInfo(mass, motionState, shape, localInertia);

		rig.rigbody = new btRigidBody(cInfo);
		rig.origin = origin;
		rig.dir = dir;
		rig.boneindex = rigcontrol->boneindex;

		return true;
	}
	else if (rigcontrol->shape == RAGDOLL_SHAPE_CAPSULE)
	{
		float height = rigcontrol->size2;

		auto bonematrix2 = bonematrix;
		bonematrix2.setOrigin(origin);
		
		btVector3 fwd(0, 1, 0);
		rigidtransform = MatrixLookAt(bonematrix2, pboneorigin, fwd);
		offsetmatrix.mult(bonematrix.inverse(), rigidtransform);

		auto shape = new btCapsuleShape(rigcontrol->size, height);

		BoneMotionState* motionState = new BoneMotionState(bonematrix, offsetmatrix);

		float mass = 1;
		btVector3 localInertia;
		shape->calculateLocalInertia(mass, localInertia);

		btRigidBody::btRigidBodyConstructionInfo cInfo(mass, motionState, shape, localInertia);

		rig.rigbody = new btRigidBody(cInfo);
		rig.origin = origin;
		rig.dir = dir;
		rig.boneindex = rigcontrol->boneindex;

		return true;
	}

	gEngfuncs.Con_Printf("CreateRigBody: Failed to create rigbody %s, invalid shape type %d\n", rigcontrol->name.c_str(), rigcontrol->shape);
	return false;
}

void CPhysicsManager::RemoveIndexedVertexArray()
{
	if (m_worldmodel_iva)
	{
		delete m_worldmodel_iva;
		m_worldmodel_iva = NULL;
	}

	for (auto iva : m_brushmodel_iva)
	{
		delete iva.second;
	}
	m_brushmodel_iva.clear();
}

void CPhysicsManager::RemoveAllStatics()
{
	for (auto p : m_staticMap)
	{
		if (p.second)
		{
			m_dynamicsWorld->removeRigidBody(p.second->m_rigbody);

			delete p.second->m_rigbody;
			delete p.second;
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

		for (auto p : ragdoll->m_rigbodyMap)
		{
			m_dynamicsWorld->removeRigidBody(p.second.rigbody);
			delete p.second.rigbody;
		}

		ragdoll->m_rigbodyMap.clear();

		delete ragdoll;
	}
	m_ragdollMap.clear();
}

void CPhysicsManager::RemoveRagdoll(int tentindex)
{
	auto itor = m_ragdollMap.find(tentindex);

	if (itor == m_ragdollMap.end())
	{
		gEngfuncs.Con_Printf("RemoveRagdoll: not found\n");
		return;
	}

	auto ragdoll = itor->second;

	for (auto p : ragdoll->m_constraintArray)
	{
		m_dynamicsWorld->removeConstraint(p);
		delete p;
	}

	ragdoll->m_constraintArray.clear();

	for (auto p : ragdoll->m_rigbodyMap)
	{
		m_dynamicsWorld->removeRigidBody(p.second.rigbody);
		delete p.second.rigbody;
	}

	ragdoll->m_rigbodyMap.clear();

	m_ragdollMap.erase(itor);

	delete ragdoll;
}

void CPhysicsManager::SetupBones(studiohdr_t *hdr, int tentindex)
{
	auto itor = m_ragdollMap.find(tentindex);

	if (itor == m_ragdollMap.end())
	{
		//gEngfuncs.Con_Printf("SetupBones: not found\n");
		return;
	}

	mstudiobone_t *pbones = (mstudiobone_t *)((byte *)(*pstudiohdr) + (*pstudiohdr)->boneindex);

	auto ragdoll = itor->second;

	for (auto &rig : ragdoll->m_rigbodyMap)
	{
		auto rigbody = rig.second.rigbody;

		auto motionState = (BoneMotionState *)rig.second.rigbody->getMotionState();

		float bonematrix[3][4];
		TransformToMatrix3x4(motionState->bonematrix, bonematrix);

		memcpy((*pbonetransform)[rig.second.boneindex], bonematrix, sizeof(float[3][4]));
		memcpy((*plighttransform)[rig.second.boneindex], bonematrix, sizeof(float[3][4]));
	}

	for (size_t index = 0; index < ragdoll->m_nonKeyBones.size(); index++)
	{
		auto i = ragdoll->m_nonKeyBones[index];
		if (i == -1)
			continue;

		auto parentmatrix3x4 = (*pbonetransform)[pbones[i].parent];
		
		btTransform parentmatrix;
		Matrix3x4ToTransform(parentmatrix3x4, parentmatrix);

		btTransform matrix;
		matrix = parentmatrix * ragdoll->m_boneRelativeTransform[i];

		TransformToMatrix3x4(matrix, (*pbonetransform)[i]);
	}
}

bool CPhysicsManager::CreateRagdoll(ragdoll_config_t *cfg, int tentindex, model_t *model, studiohdr_t *studiohdr, float *velocity)
{
	auto itor = m_ragdollMap.find(tentindex);
	
	if(itor != m_ragdollMap.end())
	{
		gEngfuncs.Con_Printf("CreateRagdoll: Already exists\n");
		return false;
	}

	std::string modelname(model->name);

	auto ragdoll = new CRagdoll();

	mstudiobone_t *pbones = (mstudiobone_t *)((byte *)(*pstudiohdr) + (*pstudiohdr)->boneindex);

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

		CRigBody rig;
		if (CreateRigBody(studiohdr, rigcontrol, rig))
		{
			ragdoll->m_keyBones.emplace_back(rigcontrol->boneindex);
			ragdoll->m_rigbodyMap[rigcontrol->name] = rig;

			btVector3 vel(velocity[0], velocity[1], velocity[2]);

			rig.rigbody->setLinearVelocity(vel);

			rig.rigbody->setFriction(0.25f);

			rig.rigbody->setRollingFriction(0.25f);

			m_dynamicsWorld->addRigidBody(rig.rigbody);
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

		auto cst = CreateConstraint(ragdoll, studiohdr, cstcontrol);

		if (cst)
		{
			ragdoll->m_constraintArray.emplace_back(cst);
			m_dynamicsWorld->addConstraint(cst, true);
		}
	}

	ragdoll->m_tentindex = tentindex;
	m_ragdollMap[tentindex] = ragdoll;

	return true;
}