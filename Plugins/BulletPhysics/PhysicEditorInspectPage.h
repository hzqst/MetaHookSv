#pragma once

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/CvarToggleCheckButton.h>
#include <vgui_controls/CvarTextEntry.h>
#include <vgui_controls/TextEntry.h>
#include <utlrbtree.h>
#include <utlsymbol.h>

class CPhysicEditorInspectPage : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(CPhysicEditorInspectPage, vgui::PropertyPage);

public:

	CPhysicEditorInspectPage(vgui::Panel* parent, const char* name);
	~CPhysicEditorInspectPage();

private:

	CCvarToggleCheckButton* m_pEnableDebugView{};
	CCvarToggleCheckButton* m_pEnableDebugViewWallHack{};
	CCvarTextEntry* m_pStaticObjectDebugDrawLevel{};
	CCvarTextEntry* m_pDynamicObjectDebugDrawLevel{};
	CCvarTextEntry* m_pRagdollObjectDebugDrawLevel{};
	CCvarTextEntry* m_pConstraintObjectDebugDrawLevel{};
	vgui::TextEntry* m_pInspectingPhysicObject{};//e.g. #2 [RagdollObject] models/player/GFL_M14-c.mdl
	vgui::TextEntry* m_pInspectingPhysicComponent{};//e.g #1 [RigidBody] Head // #2 [Constraint] NativeConstraint|Head|Pelvis
};