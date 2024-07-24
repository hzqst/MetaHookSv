#pragma once

#include <string>
#include <vector>
#include <memory>

#include "mathlib2.h"

#include "ClientPhysicCommon.h"

class CClientRigidBodyConfig
{
public:
	std::string name;
	int boneindex{ -1 };
	int parentboneindex{ -1};
	int shape{ PhysicShape_None };
	vec3_t offset{ 0 };
	float size{};
	float size2{};
	float mass{};
	int flags{};
};

class CClientPhysicConfig
{
public:
	int state{ PhysicConfigState_NotLoaded };
	std::vector<CClientRigidBodyConfig *> m_rigidBodyConfigs;
};

using CClientPhysicConfigs = std::vector<CClientPhysicConfig *>;