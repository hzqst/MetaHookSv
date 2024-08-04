#pragma once

#include <string>
#include <vector>
#include <memory>

#include "mathlib2.h"

#include "ClientPhysicCommon.h"

class CClientCollisionShapeConfig
{
public:
	std::string name;

	int type{ PhysicShape_None };

	int direction{ PhysicShapeDirection_X };

	vec3_t origin{ 0 };

	//angles only works for compound shape
	vec3_t angles{ 0 };

	vec3_t size{ 0 };

	//TODO
	//std::vector<float> multispheres;
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
	float mass{ 1 };
	float density{ 1 };
	float linearFriction{ 1 };
	float rollingFriction{ 1 };
	float restitution{ 0 };
	float ccdRadius{ 0 };
	float ccdThreshold{ 0 };
	vec3_t centerOfMass{0};
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
	float factors[16]{ 0 };
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

class CClientPhysicConfig
{
public:
	virtual ~CClientPhysicConfig()
	{
		for (auto p : RigidBodyConfigs)
		{
			delete p;
		}
		RigidBodyConfigs.clear();

		for (auto p : ConstraintConfigs)
		{
			delete p;
		}
		ConstraintConfigs.clear();
	}

public:
	int type{ PhysicConfigType_None };
	std::vector<CClientRigidBodyConfig*> RigidBodyConfigs;
	std::vector<CClientConstraintConfig*> ConstraintConfigs;
};
 
class CClientRagdollConfig : public CClientPhysicConfig
{
public:
	CClientRagdollConfig()
	{
		type = PhysicConfigType_Ragdoll;
	}

	~CClientRagdollConfig()
	{
		for (auto p : FloaterConfigs)
		{
			delete p;
		}
		FloaterConfigs.clear();
	}

	std::vector<CClientFloaterConfig*> FloaterConfigs;
	std::vector<CClientRagdollAnimControlConfig> AnimControlConfigs;
	CClientRagdollAnimControlConfig IdleAnimConfig;
};

class CClientPhysicConfigStorage
{
public:
	int state{ PhysicConfigState_NotLoaded };
	CClientPhysicConfig* pConfig{};
};

using CClientPhysicConfigs = std::vector<CClientPhysicConfigStorage>;