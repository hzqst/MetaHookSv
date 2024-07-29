#pragma once

const int PhysicConfigState_NotLoaded = 0;
const int PhysicConfigState_Loaded = 1;
const int PhysicConfigState_LoadedWithError = 2;

const int PhysicObjectFlag_Static = 1;
const int PhysicObjectFlag_Ragdoll = 2;

const int PhysicShape_None = 0;
const int PhysicShape_Box = 1;
const int PhysicShape_Sphere = 2;
const int PhysicShape_Capsule = 3;
const int PhysicShape_Cylinder = 4;
const int PhysicShape_MultiSphere = 5;

const int PhysicConstraint_None = 0;
const int PhysicConstraint_Hinge = 1;
const int PhysicConstraint_ConeTwist = 2;

const int PhysicShapeDirection_X = 0;
const int PhysicShapeDirection_Y = 1;
const int PhysicShapeDirection_Z = 2;