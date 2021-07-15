#pragma once
#include <unordered_map>
#include <vector>

#include "btBulletDynamicsCommon.h"
#include "studio.h"

typedef struct ragdoll_rig_control_s
{
	ragdoll_rig_control_s(const std::string &n, int i, int p, int sh, float off, float s, float s2, float m)
	{
		name = n;
		boneindex = i;
		pboneindex = p;
		shape = sh;
		offset = off;
		size = s;
		size2 = s2;
		mass = m;
	}
	std::string name;
	int boneindex;
	int pboneindex;
	int shape;
	float offset;
	float size;
	float size2;
	float mass;
}ragdoll_rig_control_t;

typedef struct ragdoll_cst_control_s
{
	ragdoll_cst_control_s(const std::string &n, const std::string &n2, int t, int b1, int b2, float of1, float of2, float of3, float of4, float of5, float of6, float f1, float f2, float f3)
	{
		name = n;
		linktarget = n2;

		type = t;

		boneindex1 = b1;
		boneindex2 = b2;

		offset1 = of1;
		offset2 = of2;
		offset3 = of3;
		offset4 = of4;
		offset5 = of5;
		offset6 = of6;

		factor1 = f1;
		factor2 = f2;
		factor3 = f3;
	}

	std::string name;
	std::string linktarget;

	int type;

	int boneindex1;
	int boneindex2;

	float offset1;
	float offset2;
	float offset3;

	float offset4;
	float offset5;
	float offset6;

	float factor1;
	float factor2;
	float factor3;
}ragdoll_cst_control_t;

typedef struct ragdoll_bar_control_s
{
	ragdoll_bar_control_s(const std::string &n, float x, float y, float z, int t, float f1, float f2, float f3)
	{
		name = n;
		type = t;
		offsetX = x;
		offsetY = y;
		offsetZ = z;
		factor1 = f1;
		factor2 = f2;
		factor3 = f3;
	}
	std::string name;
	int type;
	float offsetX;
	float offsetY;
	float offsetZ;
	float factor1;
	float factor2;
	float factor3;
}ragdoll_bar_control_t;

typedef struct ragdoll_config_s
{
	std::unordered_map<int, float> animcontrol;
	std::vector<ragdoll_cst_control_t> cstcontrol;
	std::vector<ragdoll_rig_control_t> rigcontrol;
	std::vector<ragdoll_bar_control_t> barcontrol;
}ragdoll_config_t;

ATTRIBUTE_ALIGNED16(class)
CRigBody
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();
	CRigBody()
	{
		rigbody = NULL;
		barnacle_constraint_dof6 = NULL;
		barnacle_constraint_slider = NULL;
		barnacle_force = 0;
		barnacle_chew_force = 0;
		barnacle_chew_duration = 0;
		barnacle_chew_time = 0;
		barnacle_chew_up_z = 0;
		barnacle_z_offset = 0;
		barnacle_z_init = 0;
		barnacle_z_final = 0;
	}
	CRigBody(const std::string &n, btRigidBody *a1, const btVector3 &a2, const btVector3 &a3, int a4) : name(n), rigbody(a1), origin(a2), dir(a3), boneindex(a4)
	{
		barnacle_constraint_dof6 = NULL;
		barnacle_constraint_slider = NULL;
		barnacle_force = 0;
		barnacle_chew_force = 0;
		barnacle_chew_duration = 0;
		barnacle_chew_time = 0;
		barnacle_chew_up_z = 0;
		barnacle_z_offset = 0;
		barnacle_z_init = 0;
		barnacle_z_final = 0;
	}
	std::string name;
	btRigidBody *rigbody;
	btGeneric6DofConstraint *barnacle_constraint_dof6;
	btSliderConstraint *barnacle_constraint_slider;
	btVector3 origin;
	btVector3 dir;
	int boneindex;
	float barnacle_force;
	float barnacle_chew_force;
	float barnacle_chew_duration;
	float barnacle_chew_time;
	float barnacle_chew_up_z;
	float barnacle_z_offset;
	float barnacle_z_init;
	float barnacle_z_final;
	btVector3 barnacle_drag_offset;
};

ATTRIBUTE_ALIGNED16(class)
CRagdoll
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();
	CRagdoll()
	{
		m_entindex = -1;
		m_barnacleindex = -1;
		m_pelvisRigBody = NULL;
		m_headRigBody = NULL;
	}

	int m_entindex;
	int m_barnacleindex;
	CRigBody *m_pelvisRigBody;
	CRigBody *m_headRigBody;
	std::vector<CRigBody *> m_barnacleDragRigBody;
	std::vector<CRigBody *> m_barnacleChewRigBody;
	std::vector<int> m_keyBones;
	std::vector<int> m_nonKeyBones;
	btTransform m_boneRelativeTransform[128];
	std::unordered_map <std::string, CRigBody *> m_rigbodyMap;
	std::vector <btTypedConstraint *> m_constraintArray;
	std::vector <btTypedConstraint *> m_barnacleConstraintArray;
};

typedef struct brushvertex_s
{
	vec3_t	pos;
}brushvertex_t;

typedef struct brushface_s
{
	//int index;
	int start_vertex;
	int num_vertexes;
}brushface_t;

typedef struct indexvertexarray_s
{
	indexvertexarray_s()
	{
		iNumFaces = 0;
		iCurFace = 0;
		iNumVerts = 0;
		iCurVert = 0;
		vVertexBuffer = NULL;
		vFaceBuffer = NULL;
	}

	int iNumFaces;
	int iCurFace;
	int iNumVerts;
	int iCurVert;
	brushvertex_t *vVertexBuffer;
	brushface_t *vFaceBuffer;
	std::vector<int> vIndiceBuffer;
}indexvertexarray_t;

ATTRIBUTE_ALIGNED16(class)
CStaticBody
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();
	CStaticBody()
	{
		m_entindex = -1;
		m_rigbody = NULL;
		m_iva = NULL;
	}
	int m_entindex;
	btRigidBody *m_rigbody;
	indexvertexarray_t *m_iva;
};

#define RAGDOLL_SHAPE_SPHERE 1
#define RAGDOLL_SHAPE_CAPSULE 2

#define RAGDOLL_CONSTRAINT_CONETWIST 1
#define RAGDOLL_CONSTRAINT_HINGE 2
#define RAGDOLL_CONSTRAINT_POINT 3

#define RAGDOLL_BARNACLE_SLIDER			1
#define RAGDOLL_BARNACLE_DOF6			2
#define RAGDOLL_BARNACLE_CHEWFORCE    3
#define RAGDOLL_BARNACLE_CHEWLIMIT    4

ATTRIBUTE_ALIGNED16(class)
BoneMotionState : public btMotionState
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();
	BoneMotionState(const btTransform &bm, const btTransform &om) : bonematrix(bm), offsetmatrix(om)
	{

	}
	virtual void getWorldTransform(btTransform& worldTrans) const;
	virtual void setWorldTransform(const btTransform& worldTrans);

	btTransform bonematrix;
	btTransform offsetmatrix;
};

#define BT_LINE_BATCH_SIZE 512

ATTRIBUTE_ALIGNED16(class)
CPhysicsDebugDraw : public btIDebugDraw
{
	int m_debugMode;

	DefaultColors m_ourColors;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	CPhysicsDebugDraw() :  m_debugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb | btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawConstraintLimits)
	{

	}

	virtual ~CPhysicsDebugDraw()
	{
	}
	virtual DefaultColors getDefaultColors() const
	{
		return m_ourColors;
	}
	///the default implementation for setDefaultColors has no effect. A derived class can implement it and store the colors.
	virtual void setDefaultColors(const DefaultColors& colors)
	{
		m_ourColors = colors;
	}

	virtual void drawLine(const btVector3& from1, const btVector3& to1, const btVector3& color1);

	virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
	{
		drawLine(PointOnB, PointOnB + normalOnB * distance, color);
		btVector3 ncolor(0, 0, 0);
		drawLine(PointOnB, PointOnB + normalOnB * 0.01, ncolor);
	}

	virtual void reportErrorWarning(const char* warningString)
	{
	}

	virtual void draw3dText(const btVector3& location, const char* textString)
	{
	}

	virtual void setDebugMode(int debugMode)
	{
		m_debugMode = debugMode;
	}

	virtual int getDebugMode() const
	{
		return m_debugMode;
	}
};

class CPhysicsManager
{
public:
	CPhysicsManager();
	void Init(void);
	void NewMap(void);
	void DebugDraw(void);
	void GenerateIndexedVertexArray(model_t *mod, indexvertexarray_t *v);
	void SetGravity(float velocity);
	void StepSimulation(double framerate);
	void ReloadConfig(void);
	ragdoll_config_t *LoadRagdollConfig(model_t *mod);
	void SetupPlayerBones(studiohdr_t *hdr, int entindex);
	void SetupBarnacleBones(studiohdr_t *hdr, int entindex);
	void RemoveRagdoll(int tentindex);
	void RemoveAllRagdolls();
	void RemoveAllStatics(); 
	bool CreateRagdoll(ragdoll_config_t *cfg, int tentindex, model_t *model, studiohdr_t *hdr, float *origin, float *velocity, bool bBarnacle, cl_entity_t *barnacle);
	CRigBody *CreateRigBody(studiohdr_t *studiohdr, ragdoll_rig_control_t *rigcontrol);
	btTypedConstraint *CreateConstraint(CRagdoll *ragdoll, studiohdr_t *hdr, ragdoll_cst_control_t *cstcontrol);
	void CreateStatic(cl_entity_t *ent, indexvertexarray_t *va);
	void CreateForBrushModel(cl_entity_t *ent);
	void CreateForBarnacle(cl_entity_t *ent);
	void RotateForEntity(cl_entity_t *ent, float matrix[4][4]);
	CRagdoll *FindRagdoll(int tentindex);
	void ReleaseRagdollFromBarnacle(CRagdoll *ragdoll);
	void UpdateTempEntity(TEMPENTITY **ppTempEntActive, double frame_time, double client_time);
	void SyncView(cl_entity_t *local, struct ref_params_s *pparams);
private:
	btDefaultCollisionConfiguration* m_collisionConfiguration;
	btCollisionDispatcher* m_dispatcher;
	btBroadphaseInterface* m_overlappingPairCache;
	btSequentialImpulseConstraintSolver* m_solver;
	btDiscreteDynamicsWorld* m_dynamicsWorld;
	CPhysicsDebugDraw *m_debugDraw;
	std::unordered_map<int, CRagdoll *> m_ragdollMap;
	std::unordered_map<int, CStaticBody *> m_staticMap;
	std::unordered_map<void *, ragdoll_config_t *> m_ragdoll_config;
};

extern CPhysicsManager gPhysicsManager;