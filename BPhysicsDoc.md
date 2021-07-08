# BulletPhysics documentation

![](https://github.com/hzqst/MetaHookSv/raw/main/img/6.png)

### Console Vars

bv_simrate 16 ~ 128 : how many times to perform simulation per persond ? higher frequency get more accurate simulation result, while eating more CPU resource.

bv_debug 0 / 1 / 2 : enable bullet engine's wireframe debug-drawing.

### How to make dead players being ragdoll

1. You have to create a file named "[model_name]_ragdoll.txt" at "Build\svencoop(_addons, _download etc)\models\[model_name]\", just like what I did in "\svencoop\models\GFL_M14\GFL_M14_ragdoll.txt". (The model is from https://gamebanana.com/mods/167065)

2. The content of "[model_name]_ragdoll.txt" should follow the format :

[DeathAnim]
12 120
(sequence number of death animation)  (which frame to transform corpse of dead player into ragdoll if this sequence of animation is playing ? 0 ~ 255)

While the specified sequence number of death animation is playing, and after the certain frame, the corpse of dead player is transformed into ragdoll.

You can check each sequence of animation from HLMV or https://github.com/danakt/web-hlmv

[RigidBody]
Head   15  6  sphere  -4.0   5.0  0.0
(internal name of rigidbody)  (the index of bone which is bound to rigidbody)  (the index of bone which is being looked at by rigidbody)  (shape of rigidbone, sphere or capsule)  (rigidbody offset from bone origin)  (size of rigidbody)  (size2 of rigidbody, "height" for capsule)

The sample line creates a rigidbody at position of "head" bone, with an offset of "-4.0" to the "neck" bone. and the size of the sphere rigidbody is 5.0

You can check name and index of each bone by setting "bv_debug" to 1 and check the game console for bone information.

[Constraint]
Head   Spine   conetwist  0.0  0.0  0.1   0.2   0.3
(rigidbody to controll)  (another rigidbody to restraint with)  (type of constraint, point or conetwist)  (constraint offset from rigidbody origin, for rigidbody1)  (constraint offset from rigidbody origin, for rigidbody2)  (factor1 of conetwist constraint)  (factor2 of conetwist constraint)  (factor3 of conetwist constraint)

The sample line creates a "conetwist" constraint for rigidbody named "Head" and "Spine", swingSpan1 limits to PI (3.141593) multiplies 0.1, swingSpan2 limits to PI (3.141593) multiplies 0.1, twistSpan limits to PI (3.141593) multiplies 0.3

### How to make dead monsters being ragdoll

Not supported yet

