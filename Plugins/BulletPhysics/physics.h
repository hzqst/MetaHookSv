#pragma once

#include <unordered_map>
#include <vector>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <studio.h>
#include <com_model.h>

enum CustomCollisionFilterGroups
{
	RagdollFilter = 0x40,
	WorldFilter = 0x80,
};

typedef struct ragdoll_anim_control_s
{
	ragdoll_anim_control_s()
	{
		sequence = 0;
		frame = 0;
		activity = 0;
	}
	ragdoll_anim_control_s(int _sequence, float _frame, int _activity)
	{
		sequence = _sequence;
		frame = _frame;
		activity = _activity;
	}
	int sequence;
	float frame;
	int activity;
}ragdoll_anim_control_t;

typedef struct ragdoll_rig_control_s
{
	ragdoll_rig_control_s(const std::string &n, int bone, int pbone, int shap, float offsetX, float offsetY, float offsetZ, float sz, float sz2, float ms, int fl)
	{
		name = n;
		boneindex = bone;
		pboneindex = pbone;
		shape = shap;
		offset[0] = offsetX;
		offset[1] = offsetY;
		offset[2] = offsetZ;
		size = sz;
		size2 = sz2;
		mass = ms;
		flags = fl;
	}
	std::string name;
	int boneindex;
	int pboneindex;
	int shape;
	float offset[3];
	float size;
	float size2;
	float mass;
	int flags;
}ragdoll_rig_control_t;

#define RIG_FL_JIGGLE 1
#define RIG_FL_KINEMATIC 2

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

typedef struct ragdoll_gar_control_s
{
	ragdoll_gar_control_s(const std::string &n, const std::string &n2, float x, float y, float z, int t, float f1, float f2, float f3)
	{
		name = n;
		name2 = n2;
		type = t;
		offsetX = x;
		offsetY = y;
		offsetZ = z;
		factor1 = f1;
		factor2 = f2;
		factor3 = f3;
	}
	std::string name, name2;
	int type;
	float offsetX;
	float offsetY;
	float offsetZ;
	float factor1;
	float factor2;
	float factor3;
}ragdoll_gar_control_t;

typedef struct ragdoll_water_control_s
{
	ragdoll_water_control_s(const std::string &n, float x, float y, float z, float f1, float f2, float f3)
	{
		name = n;
		offsetX = x;
		offsetY = y;
		offsetZ = z;
		factor1 = f1;
		factor2 = f2;
		factor3 = f3;
	}
	std::string name;
	float offsetX;
	float offsetY;
	float offsetZ;
	float factor1;
	float factor2;
	float factor3;
}ragdoll_water_control_t;

typedef struct ragdoll_config_s
{
	int state;
	std::vector<ragdoll_anim_control_t> animcontrol;
	std::vector<ragdoll_cst_control_t> cstcontrol;
	std::vector<ragdoll_rig_control_t> rigcontrol;
	std::vector<ragdoll_bar_control_t> barcontrol;
	std::vector<ragdoll_gar_control_t> garcontrol;
	std::vector<ragdoll_water_control_t> watercontrol;
}ragdoll_config_t;

ATTRIBUTE_ALIGNED16(class)
CWaterControlPoint
{
public:
	CWaterControlPoint(const btVector3 &a1, float a2, float a3, float a4) : offset(a1), buoyancy(a2)
	{
		ldamping = max(min(a3, 1.0f), 0.0f);
		adamping = max(min(a4, 1.0f), 0.0f);
	}

	btVector3 offset;
	float buoyancy;
	float ldamping;
	float adamping;
};

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
		gargantua_target = NULL;

		boneindex = -1;

		barnacle_force = 0;
		barnacle_chew_force = 0;
		barnacle_chew_duration = 0;
		barnacle_chew_time = 0;
		barnacle_chew_up_z = 0;
		barnacle_z_offset = 0;
		barnacle_z_init = 0;
		barnacle_z_final = 0;
		gargantua_force = 0;
		gargantua_drag_time = 0;

		flags = 0;
		mass = 0;
		group = btBroadphaseProxy::AllFilter;
		mask = btBroadphaseProxy::DefaultFilter | CustomCollisionFilterGroups::RagdollFilter;
		oldActivitionState = 0;
		oldCollisionFlags = 0;
	}
	/*CRigBody(const std::string &n, btRigidBody *a1, const btVector3 &a2, const btVector3 &a3, int a4) : name(n), rigbody(a1), dir(a3), boneindex(a4)
	{
		flags = 0;
		mass = 0;
		oldActivitionState = 0;
		oldCollisionFlags = 0;

		barnacle_constraint_dof6 = NULL;
		barnacle_constraint_slider = NULL;
		gargantua_target = NULL;
		barnacle_force = 0;
		barnacle_chew_force = 0;
		barnacle_chew_duration = 0;
		barnacle_chew_time = 0;
		barnacle_chew_up_z = 0;
		barnacle_z_offset = 0;
		barnacle_z_init = 0;
		barnacle_z_final = 0;
		gargantua_force = 0;
		gargantua_drag_time = 0;
	}*/
	std::string name;
	btRigidBody *rigbody;
	btGeneric6DofConstraint *barnacle_constraint_dof6;
	btSliderConstraint *barnacle_constraint_slider;
	btRigidBody *gargantua_target;
	int boneindex;

	int flags;
	float mass;
	int oldActivitionState;
	int oldCollisionFlags;
	int group;
	int mask;

	float barnacle_force;
	float barnacle_chew_force;
	float barnacle_chew_duration;
	float barnacle_chew_time;
	float barnacle_chew_up_z;
	float barnacle_z_offset;
	float barnacle_z_init;
	float barnacle_z_final;
	btVector3 barnacle_drag_offset;
	float gargantua_force;
	btVector3 gargantua_drag_offset;
	float gargantua_drag_time;

	btAlignedObjectArray<CWaterControlPoint> water_control_points;
};

ATTRIBUTE_ALIGNED16(class)
CPhysicBody
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();
	CPhysicBody()
	{
		m_entindex = -1;
	}

	int m_entindex;
};

ATTRIBUTE_ALIGNED16(class)
CRagdollBody : public CPhysicBody
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();
	CRagdollBody() : CPhysicBody()
	{
		m_barnacleindex = -1;
		m_gargantuaindex = -1;
		m_isPlayer = false;
		m_studiohdr = NULL;
		m_pelvisRigBody = NULL;
		m_iActivityType = -1;
		m_flUpdateKinematicTime = 0;
		m_bUpdateKinematic = false;
	}

	int m_barnacleindex;
	int m_gargantuaindex;
	int m_iActivityType;
	float m_flUpdateKinematicTime;
	float m_bUpdateKinematic;
	bool m_isPlayer;
	studiohdr_t *m_studiohdr;
	CRigBody *m_pelvisRigBody;
	std::vector<CRigBody *> m_barnacleDragRigBody;
	std::vector<CRigBody *> m_barnacleChewRigBody;
	std::vector<CRigBody *> m_gargantuaDragRigBody;
	std::vector<int> m_keyBones;
	std::vector<int> m_nonKeyBones;
	btTransform m_boneRelativeTransform[128];
	std::unordered_map <std::string, CRigBody *> m_rigbodyMap;
	std::vector <btTypedConstraint *> m_constraintArray;
	std::vector <btTypedConstraint *> m_barnacleConstraintArray;
	std::vector <btTypedConstraint *> m_gargantuaConstraintArray;
	std::vector<ragdoll_anim_control_t> m_animcontrol;
	std::vector<ragdoll_bar_control_t> m_barcontrol;
	std::vector<ragdoll_gar_control_t> m_garcontrol;
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

typedef struct vertexarray_s
{
	vertexarray_s()
	{
		bIsDynamic = false;
	}
	std::vector<brushvertex_t> vVertexBuffer;
	std::vector<brushface_t> vFaceBuffer;
	bool bIsDynamic;
}vertexarray_t;

typedef struct indexarray_s
{
	indexarray_s()
	{
		bIsDynamic = false;
	}
	std::vector<int> vIndiceBuffer;
	bool bIsDynamic;
}indexarray_t;

ATTRIBUTE_ALIGNED16(class)
CStaticBody : public CPhysicBody
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();
	CStaticBody() : CPhysicBody()
	{
		m_rigbody = NULL;
		m_vertexarray = NULL;
		m_indexarray = NULL;
		m_kinematic = false;
		m_debugdraw = false;
		m_group = btBroadphaseProxy::AllFilter;
		m_mask = btBroadphaseProxy::DefaultFilter | CustomCollisionFilterGroups::RagdollFilter;
	}

	btRigidBody *m_rigbody;
	vertexarray_t *m_vertexarray;
	indexarray_t *m_indexarray;
	bool m_kinematic;
	bool m_debugdraw;
	int m_group;
	int m_mask;
};

#define RAGDOLL_SHAPE_SPHERE 1
#define RAGDOLL_SHAPE_CAPSULE 2
#define RAGDOLL_SHAPE_GARGMOUTH 3

#define RAGDOLL_CONSTRAINT_CONETWIST 1
#define RAGDOLL_CONSTRAINT_HINGE 2
#define RAGDOLL_CONSTRAINT_POINT 3

#define RAGDOLL_BARNACLE_SLIDER			1
#define RAGDOLL_BARNACLE_DOF6			2
#define RAGDOLL_BARNACLE_CHEWFORCE		3
#define RAGDOLL_BARNACLE_CHEWLIMIT		4

#define RAGDOLL_GARGANTUA_SLIDER		1
#define RAGDOLL_GARGANTUA_DOF6Z			2
#define RAGDOLL_GARGANTUA_DOF6			3
#define RAGDOLL_GARGANTUA_DRAGFORCE		4

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

ATTRIBUTE_ALIGNED16(class)
EntityMotionState : public btMotionState
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	virtual void getWorldTransform(btTransform& worldTrans) const;
	virtual void setWorldTransform(const btTransform& worldTrans);

	EntityMotionState(cl_entity_t* ent) : btMotionState()
	{
		m_ent = ent;
	}

	cl_entity_t *m_ent;
};

#define BT_LINE_BATCH_SIZE 512

ATTRIBUTE_ALIGNED16(class)
CPhysicsDebugDraw : public btIDebugDraw
{
	int m_debugMode;

	DefaultColors m_ourColors;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	CPhysicsDebugDraw() :  m_debugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawConstraintLimits)
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

typedef std::unordered_map<int, CStaticBody *>::iterator static_itor;

typedef std::unordered_map<int, CRagdollBody *>::iterator ragdoll_itor;

class CPhysicsManager
{
public:
	CPhysicsManager();
	void Init(void);
	void NewMap(void);
	void DebugDraw(void);
	void GenerateBarnacleIndiceVerticeArray(void);
	void GenerateGargantuaIndiceVerticeArray(void);
	void GenerateBrushIndiceArray(void);
	void GenerateWorldVerticeArray(void);
	void GenerateIndexedArrayRecursiveWorldNode(mnode_t *node, vertexarray_t *vertexarray, indexarray_t *indexarray);
	void GenerateIndexedArrayForBrushface(brushface_t *brushface, indexarray_t *indexarray);
	void GenerateIndexedArrayForSurface(msurface_t *psurf, vertexarray_t *vertexarray, indexarray_t *indexarray);
	void GenerateIndexedArrayForBrush(model_t *mod, vertexarray_t *vertexarray, indexarray_t *indexarray);
	void SetGravity(float velocity);
	void StepSimulation(double framerate);
	void ReloadConfig(void);
	ragdoll_config_t *LoadRagdollConfig(model_t *mod);
	bool SetupBones(studiohdr_t *hdr, int entindex);
	bool SetupJiggleBones(studiohdr_t *hdr, int entindex);
	void MergeBarnacleBones(studiohdr_t *hdr, int entindex);
	bool HasRagdolls(void);
	void RemoveRagdoll(int tentindex);
	void RemoveRagdollEx(ragdoll_itor &itor);
	void RemoveAllRagdolls();
	void RemoveAllStatics(); 
	bool IsValidRagdoll(ragdoll_itor &itor);
	CRagdollBody *FindRagdoll(int tentindex);
	ragdoll_itor FindRagdollEx(int tentindex);
	bool UpdateKinematic(CRagdollBody *ragdoll, int iActivityType, entity_state_t *curstate);
	void ResetPose(CRagdollBody *ragdoll, entity_state_t *curstate);
	void ApplyBarnacle(CRagdollBody *ragdoll, cl_entity_t *barnacleEntity);
	void ApplyGargantua(CRagdollBody *ragdoll, cl_entity_t *gargEntity);
	CRagdollBody *CreateRagdoll(ragdoll_config_t *cfg, int tentindex, bool bIsPlayer);
	CRigBody *CreateRigBody(studiohdr_t *studiohdr, ragdoll_rig_control_t *rigcontrol);
	btTypedConstraint *CreateConstraint(CRagdollBody *ragdoll, studiohdr_t *hdr, ragdoll_cst_control_t *cstcontrol);
	void CreateWaterControl(CRagdollBody *ragdoll, studiohdr_t *studiohdr, ragdoll_water_control_t *water_control);
	CStaticBody *CreateStaticBody(cl_entity_t *ent, vertexarray_t *vertexarray, indexarray_t *indexarray, bool kinematic);
	void CreateBrushModel(cl_entity_t *ent);
	void CreateBarnacle(cl_entity_t *ent);
	void CreateGargantua(cl_entity_t *ent);
	void CreateWater(cl_entity_t *ent);
	void UpdateBrushTransform(cl_entity_t *ent, CStaticBody *staticBody);
	void RotateForEntity(cl_entity_t *ent, float matrix[4][4]);
	void ReleaseRagdollFromBarnacle(CRagdollBody *ragdoll);
	void ReleaseRagdollFromGargantua(CRagdollBody *ragdoll);
	bool GetRagdollOrigin(CRagdollBody *ragdoll, float *origin);
	int GetSequenceActivityType(CRagdollBody *ragdoll, entity_state_t* entstate);
	bool UpdateRagdoll(cl_entity_t *ent, CRagdollBody *ragdoll, double frame_time, double client_time);
	void UpdateRagdollWaterSimulation(cl_entity_t *ent, CRagdollBody *ragdoll, double frame_time, double client_time);
	void UpdateTempEntity(TEMPENTITY **ppTempEntActive, double frame_time, double client_time);
	void SyncPlayerView(cl_entity_t *local, struct ref_params_s *pparams);
	void PreTickCallback(btScalar timeStep);
private:
	ragdoll_itor FreeRagdollInternal(ragdoll_itor &itor);
	static_itor FreeStaticInternal(static_itor &itor);
private:
	btDefaultCollisionConfiguration* m_collisionConfiguration;
	btCollisionDispatcher* m_dispatcher;
	btBroadphaseInterface* m_overlappingPairCache;
	btSequentialImpulseConstraintSolver* m_solver;
	btDiscreteDynamicsWorld* m_dynamicsWorld;
	CPhysicsDebugDraw *m_debugDraw;
	btOverlapFilterCallback *m_overlapFilterCallback;
private:
	std::unordered_map<int, CRagdollBody *> m_ragdollMap;
	std::unordered_map<int, CStaticBody *> m_staticMap;
	std::vector<ragdoll_config_t *> m_ragdoll_config;
	std::vector<indexarray_t *> m_brushIndexArray;
	vertexarray_t *m_worldVertexArray;
	indexarray_t *m_barnacleIndexArray;
	vertexarray_t *m_barnacleVertexArray;
	indexarray_t *m_gargantuaIndexArray;
	vertexarray_t *m_gargantuaVertexArray;
	float m_gravity;
};

extern CPhysicsManager gPhysicsManager;