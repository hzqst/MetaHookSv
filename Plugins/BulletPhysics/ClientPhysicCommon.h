#pragma once

const int PhysicConfigState_NotLoaded = 0;
const int PhysicConfigState_Loaded = 1;
const int PhysicConfigState_LoadedWithError = 2;

const int PhysicConfigType_None = 0;
const int PhysicConfigType_Ragdoll = 1;

const int PhysicObjectFlag_Static = 1;
const int PhysicObjectFlag_Ragdoll = 2;
const int PhysicObjectFlag_Any = (PhysicObjectFlag_Static | PhysicObjectFlag_Ragdoll);

const int PhysicRigidBodyFlag_AlwaysDynamic = 1;
const int PhysicRigidBodyFlag_AlwaysKinematic = 2;
const int PhysicRigidBodyFlag_AlwaysStatic = 4;
const int PhysicRigidBodyFlag_NoDebugDraw = 8;
const int PhysicRigidBodyFlag_NoCollisionToOtherEntities = 0x10;//TODO

const int PhysicShape_None = 0;
const int PhysicShape_Box = 1;
const int PhysicShape_Sphere = 2;
const int PhysicShape_Capsule = 3;
const int PhysicShape_Cylinder = 4;
const int PhysicShape_MultiSphere = 5;

const int PhysicConstraint_None = 0;
const int PhysicConstraint_Hinge = 1;
const int PhysicConstraint_ConeTwist = 2;
const int PhysicConstraint_Point = 3;

const int PhysicShapeDirection_X = 0;
const int PhysicShapeDirection_Y = 1;
const int PhysicShapeDirection_Z = 2;