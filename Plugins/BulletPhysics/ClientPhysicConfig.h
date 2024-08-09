#pragma once

#include <string>
#include <vector>
#include <memory>

#include "mathlib2.h"

#include "ClientPhysicCommon.h"

class CClientCollisionShapeConfig
{
public:
	~CClientCollisionShapeConfig()
	{
		if (m_pVertexArrayStorage)
		{
			delete m_pVertexArrayStorage;
		}

		if (m_pIndexArrayStorage)
		{
			delete m_pIndexArrayStorage;
		}
	}

	std::string name;

	int type{ PhysicShape_None };

	int direction{ PhysicShapeDirection_X };

	vec3_t origin{ 0 };

	//angles only works for compound shape
	vec3_t angles{ 0 };

	vec3_t size{ 0 };

	//TODO
	//std::vector<float> multispheres;
	std::string objpath;

	CPhysicVertexArray* m_pVertexArray{};
	CPhysicIndexArray* m_pIndexArray{};

	CPhysicVertexArray* m_pVertexArrayStorage{};
	CPhysicIndexArray* m_pIndexArrayStorage{};
};

class CClientRigidBodyConfig
{
public:
	~CClientRigidBodyConfig()
	{
		for (auto p : shapes)
		{
			delete p;
		}
	}

	std::string name;

	int flags{ 0 };
	int debugDrawLevel{ 0 };

	//For positioning
	int boneindex{ -1 };
	vec3_t origin{ 0 };
	vec3_t angles{ 0 };

	//For legacy configs
	bool isLegacyConfig{ false };
	int pboneindex{ -1 };
	float pboneoffset{ 0 };

	//Support compound shape?
	std::vector<CClientCollisionShapeConfig*> shapes;

	float mass{ 1 };
	float density{ 1 };
	float linearFriction{ BULLET_DEFAULT_LINEAR_FIRCTION };
	float rollingFriction{ BULLET_DEFAULT_ANGULAR_FIRCTION  };
	float restitution{ BULLET_DEFAULT_RESTITUTION };
	float ccdRadius{ 0 };
	float ccdThreshold{ BULLET_DEFAULT_CCD_THRESHOLD };
	float linearSleepingThreshold{ BULLET_DEFAULT_LINEAR_SLEEPING_THRESHOLD };
	float angularSleepingThreshold{ BULLET_DEFAULT_ANGULAR_SLEEPING_THRESHOLD };

	//TODO?
	vec3_t centerOfMass{ 0 };
};

class CClientConstraintConfig
{
public:
	std::string name;
	int type{ PhysicConstraint_None };
	std::string rigidbodyA;
	std::string rigidbodyB;
	vec3_t origin{ 0 };
	vec3_t angles{ 0 };
	bool disableCollision{ true };
	bool isFromRigidBodyB{ false };
	int debugDrawLevel{ 0 };

	//For legacy configs
	bool isLegacyConfig{ false };
	int boneindexA{ -1 };
	int boneindexB{ -1 };
	vec3_t offsetA{ 0 };
	vec3_t offsetB{ 0 };
	float factors[32]{ NAN };
};

class CClientFloaterConfig
{
public:
	std::string rigidbody;
	vec3_t origin{};
	float buoyancy{};
	float linearDamping{};
	float angularDamping{};
};

class CClientRagdollAnimControlConfig
{
public:
	int sequence{};
	int gaitsequence{};
	float frame{};
	int activity{};
};

class CClientPhysicObjectConfig
{
public:
	virtual ~CClientPhysicObjectConfig()
	{
		for (auto p : RigidBodyConfigs)
		{
			delete p;
		}
		RigidBodyConfigs.clear();
	}

public:
	int type{ PhysicConfigType_None };
	std::vector<CClientRigidBodyConfig*> RigidBodyConfigs;
};

class CClientDynamicObjectConfig : public CClientPhysicObjectConfig
{
public:
	CClientDynamicObjectConfig()
	{
		type = PhysicConfigType_DynamicObject;
	}
	~CClientDynamicObjectConfig()
	{
		for (auto p : ConstraintConfigs)
		{
			delete p;
		}
		ConstraintConfigs.clear();
	}

	std::vector<CClientConstraintConfig*> ConstraintConfigs;
};

class CClientStaticObjectConfig : public CClientPhysicObjectConfig
{
public:
	CClientStaticObjectConfig()
	{
		type = PhysicConfigType_StaticObject;
	}

	bool isBarnacle{};
};
 
class CClientRagdollObjectConfig : public CClientPhysicObjectConfig
{
public:
	CClientRagdollObjectConfig()
	{
		type = PhysicConfigType_RagdollObject;
	}

	~CClientRagdollObjectConfig()
	{
		for (auto p : ConstraintConfigs)
		{
			delete p;
		}
		ConstraintConfigs.clear();

		for (auto p : FloaterConfigs)
		{
			delete p;
		}
		FloaterConfigs.clear();
	}

	std::vector<CClientConstraintConfig*> ConstraintConfigs;
	std::vector<CClientFloaterConfig*> FloaterConfigs;
	std::vector<CClientRagdollAnimControlConfig> AnimControlConfigs;
	CClientRagdollAnimControlConfig IdleAnimConfig;
};

class CClientPhysicObjectConfigStorage
{
public:
	int state{ PhysicConfigState_NotLoaded };
	CClientPhysicObjectConfig* pConfig{};
};

using CClientPhysicObjectConfigs = std::vector<CClientPhysicObjectConfigStorage>;