//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ICLIENTPANEL_H
#define ICLIENTPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "VGUI.h"
#include "MessageMap.h"
#include "Color.h"
#include "KeyCode.h"
#include "MouseCode.h"

#ifdef GetClassName
#undef GetClassName
#endif

class KeyValues;

namespace vgui
{

class Panel;
class BuildGroup;
class SurfaceBase;
class IBorder;
class IScheme;

enum EInterfaceID
{
	ICLIENTPANEL_STANDARD_INTERFACE = 0,
};

//-----------------------------------------------------------------------------
// Purpose: Interface from vgui panels -> Client panels
//			This interface cannot be changed without rebuilding all vgui projects
//			Primarily this interface handles dispatching messages from core vgui to controls
//			The additional functions are all their for debugging or optimization reasons
//			To add to this later, use QueryInterface() to see if they support new interfaces
//-----------------------------------------------------------------------------
class IClientPanel
{
public:
	virtual VPANEL GetVPanel() = 0;//0
	virtual void Think() = 0;//1
	virtual void PerformApplySchemeSettings() = 0;//2
	virtual void PaintTraverse(bool forceRepaint, bool allowForce) = 0;//3
	virtual void Repaint() = 0;//4
	virtual VPANEL IsWithinTraverse(int x, int y, bool traversePopups) = 0;//5
	virtual void GetInset(int &top, int &left, int &right, int &bottom) = 0;//6
	virtual void GetClipRect(int &x0, int &y0, int &x1, int &y1) = 0;//7
	virtual void OnChildAdded(VPANEL child) = 0;//8
	virtual void OnSizeChanged(int newWide, int newTall) = 0;//9
	virtual void InternalFocusChanged(bool lost) = 0;//10
	virtual bool RequestInfo(KeyValues *outputData) = 0;//11
	virtual void RequestFocus(int direction) = 0;//12
	virtual bool RequestFocusPrev(VPANEL existingPanel) = 0;//13
	virtual bool RequestFocusNext(VPANEL existingPanel) = 0;//14
	virtual void OnMessage(const KeyValues *params, VPANEL ifromPanel) = 0;//15
	virtual VPANEL GetCurrentKeyFocus() = 0;//16
	virtual int GetTabPosition() = 0;//17
	virtual const char *GetName() = 0;//18
	virtual const char *GetClassName() = 0;//19
	virtual HScheme GetScheme() = 0;//20
	virtual bool IsProportional() = 0;//21
	virtual bool IsAutoDeleteSet() = 0;//22
	virtual void DeletePanel() = 0;//23
	virtual void *QueryInterface(EInterfaceID id) = 0;//24
	virtual Panel *GetPanel() = 0;//25
	virtual const char *GetModuleName() = 0;//26
	virtual PanelMessageMap* GetMessageMap() = 0;//27
	virtual void IPanel_dtor() = 0;//28
#if !defined(_WIN32) && !defined(_WIN64)
	virtual void IPanel_dtor2() = 0;//29
#endif
	virtual void SetVisible(bool param_1) = 0;//29
	virtual bool IsVisible() = 0;//30
	virtual void PostMessage1(Panel* target, KeyValues* message, float delaySeconds) = 0;//31
	virtual void PostMessage2(VPANEL target, KeyValues* message, float delaySeconds) = 0;//32
	virtual void OnMove() = 0;//33
	virtual Panel* GetParent() = 0;//34
	virtual VPANEL GetVParent() = 0;//35
	virtual void SetParent(Panel* param_1) = 0;//36
	virtual void SetParent(VPANEL param_1) = 0;//37
	virtual bool HasParent(VPANEL potentialParent) = 0;//38
	virtual void SetAutoDelete(bool param_1) = 0;//39
	virtual void AddActionSignalTarget2(VPANEL vpanel) = 0;//40
	virtual void AddActionSignalTarget(Panel* panel) = 0;//41
	virtual void RemoveActionSignalTarget(Panel* panel) = 0;//42
	virtual void PostActionSignal(KeyValues* keyvalue) = 0;//43
	virtual bool RequestInfoFromChild(const char* childName, KeyValues* outputData) = 0;//44
	virtual void PostMessageToChild(const char* param_1, KeyValues* param_2) = 0;//45
	virtual bool SetInfo(KeyValues* inputData) = 0;//46
	virtual void SetEnabled(bool state) = 0;//47
	virtual bool IsEnabled() = 0;//48
	virtual bool IsPopup() = 0;//49
	virtual void MoveToFront() = 0;//50
	virtual void SetBgColor(Color param_1) = 0;//51
	virtual void SetFgColor(Color param_1) = 0;//52
	virtual Color GetBgColor() = 0;//53
	virtual Color GetFgColor() = 0;//54
	virtual void SetCursor(HCursor param_1) = 0;//55
	virtual HCursor GetCursor() = 0;//56
	virtual bool HasFocus() = 0;//57
	virtual void InvalidateLayout(bool param_1, bool param_2) = 0;//58
	virtual void SetTabPosition(int param_1) = 0;//59
	virtual void SetBorder(IBorder* param_1) = 0;//60
	virtual IBorder* GetBorder() = 0;//61
	virtual void SetPaintBorderEnabled(bool param_1) = 0;//62
	virtual void SetPaintBackgroundEnabled(bool param_1) = 0;//63
	virtual void SetPaintEnabled(bool param_1) = 0;//64
	virtual void SetPostChildPaintEnabled(bool param_1) = 0;//65
	virtual void GetPaintSize(int& param_1, int& param_2) = 0;//66
	virtual void SetBuildGroup(BuildGroup* param_1) = 0;//67
	virtual bool IsBuildGroupEnabled() = 0;//68
	virtual bool IsCursorNone() = 0;//69
	virtual bool IsCursorOver() = 0;//70
	virtual void MarkForDeletion() = 0;//71
	virtual bool IsLayoutInvalid() = 0;//72
	virtual Panel *HasHotkey(wchar_t key) = 0;//73
	virtual bool IsOpaque() = 0;//74
	virtual void SetScheme(HScheme param_1) = 0;//76
	virtual void SetScheme2(const char* param_1) = 0;//75
	virtual Color GetSchemeColor(const char* keyName, IScheme* pScheme) = 0;//77
	virtual Color GetSchemeColor2(const char* keyName, Color defaultColor, IScheme* pScheme) = 0;//78
	virtual void ApplySchemeSettings(IScheme* pScheme) = 0;//79
	virtual void ApplySettings(KeyValues* inResourceData) = 0;//80, actually 81 in 8684
	virtual void GetSettings(KeyValues* outResourceData) = 0;//81
	virtual const char* GetDescription() = 0;//82
	virtual void ApplyUserConfigSettings(KeyValues* userConfig) = 0;//83
	virtual void GetUserConfigSettings(KeyValues* userConfig) = 0;//84
	virtual bool HasUserConfigSettings() = 0;//85
	virtual void OnThink() = 0;//86
	virtual void OnCommand(const char* command) = 0;//87
	virtual void OnMouseCaptureLost() = 0;//88
	virtual void OnSetFocus() = 0;//89
	virtual void OnKillFocus() = 0;//90
	virtual void OnDelete() = 0;//91
	virtual void OnTick() = 0;//92
	virtual void OnCursorMoved(int x, int y) = 0;//93
	virtual void OnCursorEntered() = 0;//94
	virtual void OnCursorExited() = 0;//95
	virtual void OnMousePressed(MouseCode code) = 0;//96
	virtual void OnMouseDoublePressed(MouseCode code) = 0;//97
	virtual void OnMouseReleased(MouseCode code) = 0;//98
	virtual void OnMouseWheeled(int delta) = 0;//99
	virtual void OnKeyCodePressed(KeyCode code) = 0;//100
	virtual void OnKeyCodeTyped(KeyCode code) = 0;//101
	virtual void OnKeyTyped(wchar_t unichar) = 0;//102
	virtual void OnKeyCodeReleased(KeyCode code) = 0;//103
	virtual void OnKeyFocusTicked() = 0;//104
	virtual void OnMouseFocusTicked() = 0;//105
	virtual void PaintBackground() = 0;//106
	virtual void Paint() = 0;//107
	virtual void PaintBorder() = 0;//108
	virtual void PaintBuildOverlay() = 0;//109
	virtual void PostChildPaint() = 0;//110
	virtual void PerformLayout() = 0;//111
	virtual PanelMap_t *GetPanelMap() = 0;//112
	virtual void SetProportional(bool state) = 0;//113
	virtual void SetMouseInputEnabled(bool state) = 0;//114
	virtual void SetKeyBoardInputEnabled(bool state) = 0;//115
	virtual bool IsMouseInputEnabled() = 0;//116
	virtual bool IsKeyBoardInputEnabled() = 0;//117
	virtual void OnRequestFocus(VPANEL subFocus, VPANEL defaultPanel) = 0;//118
	virtual void InternalCursorMoved(int x, int y) = 0;//119
	virtual void InternalCursorEntered() = 0;//120
	virtual void InternalCursorExited() = 0;//121
	virtual void InternalMousePressed(int code) = 0;//122
	virtual void InternalMouseDoublePressed(int code) = 0;//123
	virtual void InternalMouseReleased(int code) = 0;//124
	virtual void InternalMouseWheeled(int code) = 0;//125
	virtual void InternalKeyCodePressed(int code) = 0;//126
	virtual void InternalKeyCodeTyped(int code) = 0;//127
	virtual void InternalKeyTyped(int code) = 0;//128
	virtual void InternalKeyCodeReleased(int code) = 0;//129
	virtual void InternalKeyFocusTicked() = 0;//130
	virtual void InternalMouseFocusTicked() = 0;//131
	virtual void InternalInvalidateLayout() = 0;//132
	virtual void InternalMove() = 0;//133

};

} // namespace vgui


#endif // ICLIENTPANEL_H
