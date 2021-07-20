# BulletPhysics documentation

[中文DOC](BulletPhysicsCN.md)

### 功能

1. Transform player model into ragdoll when player is dead or being caught by barnacle.

2. Transform monster model into ragdoll when monster is dead. (Not supported yet)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/6.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/7.png)

### Compatibility

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | -    |
| GoldSrc_legacy (< 6153)  | -    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |

For GoldSrc engine: Only compatible with metahook launcher (metahook.exe) from https://github.com/hzqst/MetaHookSv, not from other source.

### Console Vars

bv_simrate 32 ~ 128 : How many times to perform simulation per persond ? higher simulation frequency get more accurate result, while eating more CPU resource. default : 64

bv_scale : Scaling the size of world insize bullet-engine. default : 0.25, both too-large and too-small dynamic collision objects in bullet-engine world will be unstable.

bv_debug 1 : Enable bullet-engine's debug-drawing. only ragdoll rigidbodies are drawn.

bv_debug 2 : Enable bullet-engine's debug-drawing. both ragdoll, brush-entity rigidbodies are drawn.

bv_debug 3 : Enable bullet-engine's debug-drawing. both ragdoll, world rigidbodies are drawn.

bv_debug 4 : Enable bullet-engine's debug-drawing. both ragdoll rigidbodies and conetwist constraints are drawn.

bv_debug 5 : Enable bullet-engine's debug-drawing. both ragdoll rigidbodies and hinge constraints are drawn.

bv_debug 6 : Enable bullet-engine's debug-drawing. both ragdoll rigidbodies and point constraints are drawn.

### How to make dead players being ragdoll

1. You have to create a file named "[model_name]_ragdoll.txt" at "\steamapps\common\Sven Co-op\svencoop(_addons, _download etc)\models\[model_name]\", just like what I did in "Build\svencoop\models\player\GFL_M14\GFL_M14_ragdoll.txt". (The model is from https://gamebanana.com/mods/167065)

2. The content of "[model_name]_ragdoll.txt" should follow the format :

#### [DeathAnim]

##### example: 12 120

"12" for the sequence number of death animation 

"120" for which frame to transform corpse of dead player into ragdoll if this sequence of animation is playing. range : 0 ~ 255

** While the specified sequence number of death animation is playing, and after the certain frame, the corpse of dead player is transformed into ragdoll. **

You can check each sequence of animation from HLMV or https://github.com/danakt/web-hlmv

#### [RigidBody]

##### example: Head   15  6  sphere  -4.0   5.0  0.0   6

"Head" for internal name of rigidbody

"15" for the index of bone which is bound to rigidbody

"6" for the index of bone which is being looked at by rigidbody

"sphere" for shape of rigidbone, either sphere or capsule.

"-4.0" for rigidbody Z offset from bone origin

"5.0" for size of rigidbody

"0.0" for size2 of rigidbody, "height" for capsule

"6" for rigidbody mass

** The example line creates a rigidbody named "Head" at position of bone[15], with an offset of "-4.0" to the bone[6]. and the size of sphere rigidbody is 5.0 **

** You can check bone information of studiomodel by setting "bv_debug" to 1 and check the game console for bone information.**

** Make sure _ragdoll.txt is in the correct location if you got nothing in the console.**

#### [Constraint]

##### example: Spine  Head   conetwist 6 15   0 6 0     0  0 -5.5      0.05 0.05 0.2

"Head" for the name of rigidbody to controll

"Spine" for the name of another rigidbody to controll

"conetwist" for type of constraint, point or conetwist or hinge.

"6" for bone index to calculate origin offset from center of "Head"

"15" for bone index 2 to calculate origin offset from center of "Spine"

"0" "6" "0" for local offset X, Y, Z from origin of bone[6]

"0" "0" "-5.5" for local offset X, Y, Z from origin of bone[15]

"0.05" "0.05" "0.2" for factors of current constraint.

** The example line creates a "conetwist" constraint for rigidbodies named "Head" and "Spine", swingSpan1 limits to PI (3.141593) multiplies 0.05, swingSpan2 limits to PI (3.141593) multiplies 0.05, twistSpan limits to PI (3.141593) multiplies 0.2 **

#### [Barnacle]

##### example: LLeg2   dof6     0  8  0     40000    4  16

"LLeg2" for the name of rigidbody to controll

"dof6" for type of constraint( dof6 / slider ) or chew (which will apply a force every X seconds).

"0" "8" "0" for local offset X, Y, Z from origin of LLeg2

"40000" for magnitude of force to pull ragdoll up.

"4" for final height (Z axis) offset of rigidbody height limit in GoldSrc world.

"16" for initial height (Z axis) offset to "4", the initial offset is 4 - 16 = -12

** This example line creates a "dof6" constraint for "LLeg2", pulling "LLeg2" up with a power/force of 40000 N. "LLeg2" can go up to (barnacle.z - 12) at initial. **

##### example2: Pelvis  chewforce     0  0  0     8000     1.0  0

"Pelvis" for the name of rigidbody to apply impulse on

"0" "0" "0" for local offset X, Y, Z from origin of Pelvis, aka center.

"8000" for magnitude of impulse to pull ragdoll up.

"1.0" for every 1 second the Pelvis is applied a impulse of 5000 unit.

0 for nothing.

** This example line applies an impulse force of 8000 to "Pelvis" every 1 second only when barnacle is chewing. **

** Warning : there must be a rigidbody named "Pelvis" to get barnacle-ragdoll work correctly. **

##### example2: LLeg2  chewlimit     0  0  0     0     1.0  3

"LLeg2" for the name of rigidbody to raise up the Z limit.

"0" "0" "0" for nothing.

"0" for nothing.

"1.0"  "3" for every 1 second the Pelvis's Z axis limit is raised up by 3.

** This example line raise the Z axis limit up 3 units for "Pelvis" every 1 second in GoldSrc world. **

** Warning : there must be a rigidbody named "Pelvis" to get barnacle-ragdoll work correctly. **

### How to make dead monsters being ragdoll

Not supported yet
