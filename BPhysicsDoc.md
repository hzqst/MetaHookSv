# BulletPhysics documentation

![](https://github.com/hzqst/MetaHookSv/raw/main/img/6.png)

### Console Vars

bv_simrate 16 ~ 128 : how many times to perform simulation per persond ? higher simulation frequency get more accurate result, while eating more CPU resource.

bv_scale : scaling the size of world insize bullet-engine.

bv_debug 1 : enable bullet-engine's debug-drawing. only ragdoll rigidbodies are drawn.

bv_debug 2 : enable bullet-engine's debug-drawing. both ragdoll, brush-entity rigidbodies are drawn.

bv_debug 3 : enable bullet-engine's debug-drawing. both ragdoll, world rigidbodies are drawn.

bv_debug 4 : enable bullet-engine's debug-drawing. both ragdoll rigidbodies and conetwist constraints are drawn.

bv_debug 5 : enable bullet-engine's debug-drawing. both ragdoll rigidbodies and hinge constraints are drawn.

bv_debug 6 : enable bullet-engine's debug-drawing. both ragdoll rigidbodies and point constraints are drawn.

### How to make dead players being ragdoll

1. You have to create a file named "[model_name]_ragdoll.txt" at "\steamapps\common\Sven Co-op\svencoop(_addons, _download etc)\models\[model_name]\", just like what I did in "Build\svencoop\models\GFL_M14\GFL_M14_ragdoll.txt". (The model is from https://gamebanana.com/mods/167065)

2. The content of "[model_name]_ragdoll.txt" should follow the format :

[DeathAnim]

12 120

(sequence number of death animation)

(which frame to transform corpse of dead player into ragdoll if this sequence of animation is playing ? 0 ~ 255)

While the specified sequence number of death animation is playing, and after the certain frame, the corpse of dead player is transformed into ragdoll.

You can check each sequence of animation from HLMV or https://github.com/danakt/web-hlmv

[RigidBody]

Head   15  6  sphere  -4.0   5.0  0.0   6

"Head" for internal name of rigidbody

"15" for the index of bone which is bound to rigidbody

"6" for the index of bone which is being looked at by rigidbody

"sphere" for shape of rigidbone, either sphere or capsule.

"-4.0" for rigidbody Z offset from bone origin

"5.0" for size of rigidbody

"0.0" for size2 of rigidbody, "height" for capsule

"6" for rigidbody mass

The sample line creates a rigidbody at position of "head" bone, with an offset of "-4.0" to the "neck" bone. and the size of the sphere rigidbody is 5.0

You can check name and index of each bone by setting "bv_debug" to 1 and check the game console for bone information.

[Constraint]

Spine  Head   conetwist 6 15   0 6 0     0  0 -5.5      0.05 0.05 0.2

"Head" for the name of rigidbody to controll

"Spine" for the name of another rigidbody to controll

"conetwist" for type of constraint, point or conetwist or hinge.

"6" for bone index to calculate origin offset from center of "Head"

"15" for bone index 2 to calculate origin offset from center of "Spine"

"0" "6" "0" for local offset X, Y, Z from origin of bone[6]

"0" "0" "-5.5" for local offset X, Y, Z from origin of bone[15]

"0.05" "0.05" "0.2" for factors of current constraint.

The sample line creates a "conetwist" constraint for rigidbody named "Head" and "Spine", swingSpan1 limits to PI (3.141593) multiplies 0.05, swingSpan2 limits to PI (3.141593) multiplies 0.05, twistSpan limits to PI (3.141593) multiplies 0.2

### How to make dead monsters being ragdoll

Not supported yet