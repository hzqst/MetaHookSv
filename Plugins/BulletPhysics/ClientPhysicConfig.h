#pragma once

#include <string>
#include <vector>
#include <memory>

#include "mathlib2.h"

#include "ClientPhysicCommon.h"

class CClientBasePhysicConfig : public IBaseInterface
{
public:
	CClientBasePhysicConfig();
	~CClientBasePhysicConfig();

	int configId{};
	int configType{ PhysicConfigType_None };
	bool configModified{};
};

class CClientCollisionShapeConfig;

using CClientCollisionShapeConfigSharedPtr = std::shared_ptr<CClientCollisionShapeConfig>;
using CClientCollisionShapeConfigs = std::vector< CClientCollisionShapeConfigSharedPtr>;

class CClientCollisionShapeConfig : public CClientBasePhysicConfig
{
public:
	CClientCollisionShapeConfig();

	int type{ PhysicShape_None };

	int direction{ PhysicShapeDirection_Y };

	vec3_t size{ 0 };

	//for compound shape
	bool is_child{};

	//for compound shape
	vec3_t origin{ 0 };

	//for compound shape
	vec3_t angles{ 0 };

	//TODO for multi sphere
	//std::vector<vec4_t> multispheres;

	std::string resourcePath;

	CClientCollisionShapeConfigs compoundShapes;
};

class CClientRigidBodyConfig : public CClientBasePhysicConfig
{
public:
	CClientRigidBodyConfig();

	std::string name;

	int flags{ PhysicRigidBodyFlag_None };
	int debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };

	//For positioning
	int boneindex{ -1 };
	vec3_t origin{ 0 };
	vec3_t angles{ 0 };

	//For legacy configs
	bool isLegacyConfig{ false };
	int pboneindex{ -1 };
	float pboneoffset{ 0 };
	vec3_t forward{ 0, 1, 0 };

	float mass{ 1 };
	float density{ 1 };
	float linearFriction{ BULLET_DEFAULT_LINEAR_FRICTION };
	float rollingFriction{ BULLET_DEFAULT_ANGULAR_FRICTION  };
	float restitution{ BULLET_DEFAULT_RESTITUTION };
	float ccdRadius{ 0 };
	float ccdThreshold{ BULLET_DEFAULT_CCD_THRESHOLD };
	float linearSleepingThreshold{ BULLET_DEFAULT_LINEAR_SLEEPING_THRESHOLD };
	float angularSleepingThreshold{ BULLET_DEFAULT_ANGULAR_SLEEPING_THRESHOLD };

	//TODO?
	//vec3_t centerOfMass{ 0 };

	CClientCollisionShapeConfigSharedPtr collisionShape;
};

class CClientConstraintConfig : public CClientBasePhysicConfig
{
public:
	CClientConstraintConfig();

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

	float maxTolerantLinearError{ BULLET_DEFAULT_MAX_TOLERANT_LINEAR_ERROR };

	//For legacy configs
	bool isLegacyConfig{ false };
	int boneindexA{ -1 };
	int boneindexB{ -1 };
	vec3_t offsetA{ 0 };
	vec3_t offsetB{ 0 };
};

class CClientPhysicActionConfig : public CClientBasePhysicConfig
{
public:
	CClientPhysicActionConfig();

	int type{ PhysicAction_None };
	std::string name;
	std::string rigidbody;
	std::string constraint;
	vec3_t origin{ 0 };
	vec3_t angles{ 0 };
	int flags{ 0 };
	int debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };
	float factors[PhysicActionFactorIdx_Maximum]{  };
};

class CClientAnimControlConfig : public CClientBasePhysicConfig
{
public:
	int sequence{};
	int gaitsequence{};
	float frame{};
	StudioAnimActivityType activity{ StudioAnimActivityType_Idle };
	int controller[4]{ 0 };
	int blending[4]{ 0 };
};

class CClientPhysicObjectConfig : public CClientBasePhysicConfig
{
public:
	CClientPhysicObjectConfig();

	int type{ PhysicObjectType_None };
	
	int flags{}; //runtime flags that used by physic engine

	int debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };

	bool verifyBoneChunk{};
	bool verifyModelFile{};
	std::string crc32BoneChunk{};
	std::string crc32ModelFile{};

	std::vector<std::shared_ptr<CClientRigidBodyConfig>> RigidBodyConfigs;
	std::vector<std::shared_ptr<CClientConstraintConfig>> ConstraintConfigs;
	std::vector<std::shared_ptr<CClientPhysicActionConfig>> ActionConfigs;

	//Never save to file or load from file
	std::string modelName;
	std::string shortName;
};

class CClientDynamicObjectConfig : public CClientPhysicObjectConfig
{
public:
	CClientDynamicObjectConfig();
};

class CClientStaticObjectConfig : public CClientPhysicObjectConfig
{
public:
	CClientStaticObjectConfig();
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
	CClientRagdollObjectConfig();

	std::vector<std::shared_ptr<CClientAnimControlConfig>> AnimControlConfigs;
	CClientCameraControlConfig FirstPersonViewCameraControlConfig;
	CClientCameraControlConfig ThirdPersonViewCameraControlConfig;
};

class CClientPhysicObjectConfigStorage
{
public:
	int state{ PhysicConfigState_NotLoaded };
	std::string modelname;
	std::shared_ptr<CClientPhysicObjectConfig> pConfig{};
};

using CClientPhysicObjectConfigs = std::vector<CClientPhysicObjectConfigStorage>;