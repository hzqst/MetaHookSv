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
			m_pVertexArrayStorage = nullptr;
		}

		if (m_pIndexArrayStorage)
		{
			delete m_pIndexArrayStorage;
			m_pVertexArrayStorage = nullptr;
		}
	}

	std::string name;

	int type{ PhysicShape_None };

	int direction{ PhysicShapeDirection_Y };

	vec3_t origin{ 0 };

	//angles only works for compound shape
	vec3_t angles{ 0 };

	vec3_t size{ 0 };

	//TODO
	//std::vector<float> multispheres;
	std::string objpath;

	CPhysicVertexArray* m_pVertexArray{};
	CPhysicIndexArray* m_pIndexArray{};

	//TODO: Put this shit to dedicated storage
	CPhysicVertexArray* m_pVertexArrayStorage{};
	CPhysicIndexArray* m_pIndexArrayStorage{};
};

class CClientRigidBodyConfig
{
public:
	std::string name;

	int flags{ PhysicRigidBodyFlag_None };
	int debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };

	//For positioning
	int boneindex{ -1 };
	vec3_t origin{ 0 };
	vec3_t angles{ 0 };
	vec3_t forward{ 0, 1, 0 };

	//For legacy configs
	bool isLegacyConfig{ false };
	int pboneindex{ -1 };
	float pboneoffset{ 0 };

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
	//vec3_t centerOfMass{ 0 };

	//Support compound shape?
	std::vector<std::shared_ptr<CClientCollisionShapeConfig>> shapes;
};

class CClientConstraintConfig
{
public:
	CClientConstraintConfig()
	{
		for (int i = 0; i < _ARRAYSIZE(factors); ++i) {
			factors[i] = NAN;
		}
	}

	std::string name;
	int type{ PhysicConstraint_None };
	std::string rigidbodyA;
	std::string rigidbodyB;
	vec3_t originA{ 0 };
	vec3_t anglesA{ 0 };
	vec3_t originB{ 0 };
	vec3_t anglesB{ 0 };
	vec3_t forward{ 0, 1, 0 };

	bool disableCollision{ true };
	bool useGlobalJointFromA{ true };
	bool useLookAtOther{ false };
	bool useGlobalJointOriginFromOther{ false };
	bool useRigidBodyDistanceAsLinearLimit{ false };
	bool useLinearReferenceFrameA{ true };
	int rotOrder{ PhysicRotOrder_XYZ };

	int flags{ 0 };
	int debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };
	float factors[PhysicConstraintFactorIdx_Maximum]{  };

	float maxTolerantLinearError{ BULLET_MAX_TOLERANT_LINEAR_ERROR };

	//For legacy configs
	bool isLegacyConfig{ false };
	int boneindexA{ -1 };
	int boneindexB{ -1 };
	vec3_t offsetA{ 0 };
	vec3_t offsetB{ 0 };
};

class CClientPhysicActionConfig
{
public:
	CClientPhysicActionConfig()
	{
		for (int i = 0; i < _ARRAYSIZE(factors); ++i) {
			factors[i] = NAN;
		}
	}

	int type{ PhysicAction_None };
	std::string name;
	std::string rigidbodyA;
	std::string rigidbodyB;
	std::string constraint;
	int flags{ 0 };
	float factors[PhysicActionFactorIdx_Maximum]{  };
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

class CClientAnimControlConfig
{
public:
	int sequence{};
	int gaitsequence{};
	float frame{};
	int activity{};
};

class CClientPhysicObjectConfig : public IBaseInterface
{
public:
	int type{ PhysicConfigType_None };
	int flags{};
	std::vector<std::shared_ptr<CClientRigidBodyConfig>> RigidBodyConfigs;
};

class CClientDynamicObjectConfig : public CClientPhysicObjectConfig
{
public:
	CClientDynamicObjectConfig()
	{
		type = PhysicConfigType_DynamicObject;
		flags = PhysicObjectFlag_DynamicObject;
	}

	std::vector<std::shared_ptr<CClientConstraintConfig>> ConstraintConfigs;
};

class CClientStaticObjectConfig : public CClientPhysicObjectConfig
{
public:
	CClientStaticObjectConfig()
	{
		type = PhysicConfigType_StaticObject;
		flags = PhysicObjectFlag_StaticObject;
	}
};

class CClientBarnacleControlConfig
{
public:
	std::vector<std::shared_ptr<CClientConstraintConfig>> ConstraintConfigs;
	std::vector<std::shared_ptr<CClientPhysicActionConfig>> ActionConfigs;
};

class CClientCameraControlConfig
{
public:
	std::string rigidbody;
	vec3_t origin{0};
	vec3_t angles{0};
};

class CClientRagdollObjectConfig : public CClientPhysicObjectConfig
{
public:
	CClientRagdollObjectConfig()
	{
		type = PhysicConfigType_RagdollObject;
		flags = PhysicObjectFlag_RagdollObject;
		FirstPersionViewCameraControlConfig.rigidbody = "Head";
		ThirdPersionViewCameraControlConfig.rigidbody = "Pelvis";
	}

	std::vector<std::shared_ptr<CClientConstraintConfig>> ConstraintConfigs;
	std::vector<std::shared_ptr<CClientFloaterConfig>> FloaterConfigs;
	std::vector<CClientAnimControlConfig> AnimControlConfigs;
	CClientAnimControlConfig IdleAnimConfig;
	CClientBarnacleControlConfig BarnacleControlConfig;
	CClientCameraControlConfig FirstPersionViewCameraControlConfig;
	CClientCameraControlConfig ThirdPersionViewCameraControlConfig;
};

class CClientPhysicObjectConfigStorage
{
public:
	int state{ PhysicConfigState_NotLoaded };
	std::shared_ptr<CClientPhysicObjectConfig> pConfig{};
};

using CClientPhysicObjectConfigs = std::vector<CClientPhysicObjectConfigStorage>;