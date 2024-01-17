#pragma once

#include "PhysicManager.h"

class CBasePhysicManager : public IPhysicManager
{
public:
	void Init(void) override;
	void Shutdown() override;
	void NewMap(void) override;
	void DebugDraw(void) override;
	void SetGravity(float velocity) override;
	void StepSimulation(double framerate) override;
	void ReloadConfig(void)  override;
	bool SetupBones(studiohdr_t* hdr, int entindex)  override;
	bool SetupJiggleBones(studiohdr_t* hdr, int entindex)  override;
	void MergeBarnacleBones(studiohdr_t* hdr, int entindex) override;
	bool ChangeRagdollEntityIndex(int old_entindex, int new_entindex) override;
	IRagdollObject* FindRagdollObject(int entindex) override;
	IRagdollObject* CreateRagdollObject(model_t* mod, int entindex, const CRagdollConfig* config) override;
	IStaticObject* CreateStaticObject(model_t* mod, int entindex) override;
	void CreateBrushModel(cl_entity_t* ent) override;
	void CreateBarnacle(cl_entity_t* ent) override;
	void CreateGargantua(cl_entity_t* ent) override;
	void RemovePhysicObject(int entindex) override;
	void UpdateTempEntity(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time) override;
};