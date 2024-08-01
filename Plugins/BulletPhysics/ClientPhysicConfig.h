#pragma once

#include <string>
#include <vector>
#include <memory>

#include "mathlib2.h"

#include "ClientPhysicCommon.h"

class CBasePhysicConfig
{
public:
	virtual ~CBasePhysicConfig()
	{

	}

	std::string name;
};

class CClientCollisionShapeConfig : public CBasePhysicConfig
{
public:
	int type{ PhysicShape_None };

	int direction{ PhysicShapeDirection_X };

	vec3_t origin{ 0 };

	//angles only works for compound shape
	vec3_t angles{ 0 };

	vec3_t size{ 0 };

	//TODO
	//std::vector<float> multispheres;
};

using CClientCollisionShapeConfigSharedPtr = std::shared_ptr<CClientCollisionShapeConfig>;

class CClientRigidBodyConfig : public CBasePhysicConfig
{
public:
	float mass{};
	float density{};
	float linearfriction{};
	float rollingfriction{};
	float restitution{};
	float ccdradius{};
	float ccdthreshold{};
	int flags{};
	vec3_t centerofmass;

	//Support compound shape?
	std::vector<CClientCollisionShapeConfigSharedPtr> shapes;

	//Relocate with bone if boneindex >= 0
	int boneindex{ -1 };
	vec3_t origin{};
	vec3_t angles{};
};

using CClientRigidBodyConfigSharedPtr = std::shared_ptr<CClientRigidBodyConfig>;

class CClientConstraintConfig : public CBasePhysicConfig
{
public:
	int type{ PhysicConstraint_None };
	std::string rigidbodyA;
	std::string rigidbodyB;
	int boneindex{ -1 };
	vec3_t origin{};
	vec3_t angles{};
	bool disableCollision{ true };
};

using CClientConstraintConfigSharedPtr = std::shared_ptr<CClientConstraintConfig>;

class CClientFloaterConfig : public CBasePhysicConfig
{
public:
	std::string rigidbody;
	vec3_t origin{};
	float buoyancy{};
	float linearDamping{};
	float angularDamping{};
};

using CClientFloaterConfigSharedPtr = std::shared_ptr<CClientFloaterConfig>;

class CClientRagdollAnimControlConfig : public CBasePhysicConfig
{
public:
	int sequence{};
	float frame{};
	int activity{};
};

class CClientPhysicConfig : public CBasePhysicConfig
{
public:
	int type{ PhysicConfigType_None };
	int state{ PhysicConfigState_NotLoaded };
	int sequence{ 0 };
	int gaitsequence{ 0 };
	std::vector<CClientRigidBodyConfigSharedPtr> rigidBodyConfigs;
	std::vector<CClientConstraintConfigSharedPtr> constraintConfigs;
	std::vector<CClientFloaterConfigSharedPtr> floaterConfigs;
	std::vector<CClientRagdollAnimControlConfig> ragdollAnimControlConfigs;
};

using CClientPhysicConfigSharedPtr = std::shared_ptr<CClientPhysicConfig>;

using CClientPhysicConfigs = std::vector<CClientPhysicConfigSharedPtr>;