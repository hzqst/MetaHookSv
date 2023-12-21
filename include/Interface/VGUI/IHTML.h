#ifndef IHTML_H
#define IHTML_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui/MouseCode.h>
#include <vgui/KeyCode.h>
#include <vgui/IImage.h>

namespace vgui
{

class IHTML
{
public:
	enum MOUSE_STATE { UP, DOWN, MOVE };

public:
	virtual void OpenURL(const char *url) = 0;
	virtual bool StopLoading(void) = 0;
	virtual bool Refresh(void) = 0;
	virtual bool Show(bool shown) = 0;
	virtual char *GetOpenedPage(void) = 0;
	virtual void OnSize(int x, int y, int w, int h) = 0;
	virtual void GetHTMLSize(int &wide, int &tall) = 0;
	virtual void Clear(void) = 0;
	virtual void AddText(const char *text) = 0;
	virtual void OnMouse(MouseCode code, MOUSE_STATE s, int x, int y) = 0;
	virtual void OnChar(wchar_t unichar) = 0;
	virtual void OnKeyDown(KeyCode code) = 0;
	virtual IImage *GetBitmap(void) = 0;
	virtual void SetVisible(bool state) = 0;
};

class IHTMLEvents
{
public:
	virtual bool OnStartURL(const char *url, const char *target, bool first) = 0;
	virtual void OnFinishURL(const char *url) = 0;
	virtual void OnProgressURL(long current, long maximum) = 0;
	virtual void OnSetStatusText(const char *text) = 0;
	virtual void OnUpdate(void) = 0;
	virtual void OnLink(void) = 0;
	virtual void OffLink(void) = 0;
};
}

#endif