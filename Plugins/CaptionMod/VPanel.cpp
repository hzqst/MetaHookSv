#include <stdio.h>

#include <vgui/IPanel.h>
#include <vgui/IClientPanel.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/Cursor.h>
#include <VGUI_controls/Controls.h>
#include "vgui_internal.h"
#include "VPanel.h"

using namespace vgui;

VPanel::VPanel(void)
{
	_pos[0] = _pos[1] = 0;
	_absPos[0] = _absPos[1] = 0;
	_size[0] = _size[1] = 0;

	_minimumSize[0] = 0;
	_minimumSize[1] = 0;

	_zpos = 0;

	_inset[0] = _inset[1] = _inset[2] = _inset[3] = 0;
	_clipRect[0] = _clipRect[1] = _clipRect[2] = _clipRect[3] = 0;

	_visible = true;
	_enabled = true;
	_clientPanel = NULL;
	_parent = NULL;
	_plat = NULL;
	_popup = false;
	_popupVisible = false;
	_isTopmostPopup = false;
	_listEntry = INVALID_PANELLIST;

	_mouseInput = true;
	_kbInput = true;
}

VPanel::~VPanel(void)
{
}

void VPanel::Init(IClientPanel *attachedClientPanel)
{
	_clientPanel = attachedClientPanel;
}

void VPanel::Solve(void)
{
	int absX = _pos[0];
	int absY = _pos[1];
	_absPos[0] = _pos[0];
	_absPos[1] = _pos[1];

	VPanel *parent = GetParent();

	if (IsPopup())
		parent = (VPanel *)surface()->GetEmbeddedPanel();

	int pinset[4] = { 0, 0, 0, 0 };

	if (parent)
	{
		parent->GetInset(pinset[0], pinset[1], pinset[2], pinset[3]);

		int pabsX, pabsY;
		parent->GetAbsPos(pabsX, pabsY);

		absX += pabsX + pinset[0];
		absY += pabsY + pinset[1];

		_absPos[0] = clamp(absX, -32767, 32767);
		_absPos[1] = clamp(absY, -32767, 32767);
	}

	_clipRect[0] = _absPos[0];
	_clipRect[1] = _absPos[1];

	int wide, tall;
	GetSize(wide, tall);

	int absX2 = absX + wide;
	int absY2 = absY + tall;

	_clipRect[2] = clamp(absX2, -32767, 32767);
	_clipRect[3] = clamp(absY2, -32767, 32767);

	if (parent && !IsPopup())
	{
		int pclip[4];
		parent->GetClipRect(pclip[0], pclip[1], pclip[2], pclip[3]);

		if (_clipRect[0] < pclip[0])
			_clipRect[0] = pclip[0];

		if (_clipRect[1] < pclip[1])
			_clipRect[1] = pclip[1];

		if (_clipRect[2] > pclip[2])
			_clipRect[2] = pclip[2] - pinset[2];

		if (_clipRect[3] > pclip[3])
			_clipRect[3] = pclip[3] - pinset[3];
	}
}

void VPanel::SetPos(int x, int y)
{
	_pos[0] = x;
	_pos[1] = y;
}

void VPanel::GetPos(int &x, int &y)
{
	x = _pos[0];
	y = _pos[1];
}

void VPanel::SetSize(int wide, int tall)
{
	if (wide < _minimumSize[0])
		wide = _minimumSize[0];

	if (tall < _minimumSize[1])
		tall = _minimumSize[1];

	if (_size[0] == wide && _size[1] == tall)
		return;

	_size[0] = wide;
	_size[1] = tall;

	Client()->OnSizeChanged(wide, tall);
}

HPanelList VPanel::GetListEntry(void)
{
	return _listEntry;
}

void VPanel::SetListEntry(HPanelList listEntry)
{
	_listEntry = listEntry;
}

bool VPanel::Render_IsPopupPanelVisible(void)
{
	return _popupVisible;
}

void VPanel::Render_SetPopupVisible(bool state)
{
	_popupVisible = state;
}

void VPanel::GetSize(int &wide,int &tall)
{
	wide = _size[0];
	tall = _size[1];
}

void VPanel::SetMinimumSize(int wide, int tall)
{
	_minimumSize[0] = wide;
	_minimumSize[1] = tall;

	int currentWidth = _size[0];

	if (currentWidth < wide)
		currentWidth = wide;

	int currentHeight = _size[1];

	if (currentHeight < tall)
		currentHeight = tall;

	if (currentWidth != _size[0] || currentHeight != _size[1])
		SetSize(currentWidth, currentHeight);
}

void VPanel::GetMinimumSize(int &wide, int &tall)
{
	wide = _minimumSize[0];
	tall = _minimumSize[1];
}

void VPanel::SetVisible(bool state)
{
	if (_visible == state)
		return;

	surface()->SetPanelVisible((VPANEL)this, state);

	_visible = state;

	if (IsPopup())
		vgui::surface()->CalculateMouseVisible();
}

void VPanel::SetEnabled(bool state)
{
	_enabled = state;
}

bool VPanel::IsVisible(void)
{
	return _visible;
}

bool VPanel::IsEnabled(void)
{
	return _enabled;
}

void VPanel::GetAbsPos(int &x, int &y)
{
	x = _absPos[0];
	y = _absPos[1];
}

void VPanel::GetClipRect(int &x0, int &y0, int &x1, int &y1)
{
	x0 = _clipRect[0];
	y0 = _clipRect[1];
	x1 = _clipRect[2];
	y1 = _clipRect[3];
}

void VPanel::SetInset(int left, int top, int right, int bottom)
{
	_inset[0] = left;
	_inset[1] = top;
	_inset[2] = right;
	_inset[3] = bottom;
}

void VPanel::GetInset(int &left, int &top, int &right, int &bottom)
{
	left = _inset[0];
	top = _inset[1];
	right = _inset[2];
	bottom = _inset[3];
}

void VPanel::SetParent(VPanel *newParent)
{
	if (this == newParent)
		return;

	if (_parent == newParent)
		return;

	if (_parent != NULL)
	{
		_parent->_childDar.RemoveElement(this);
		_parent = null;
	}

	if (newParent != NULL)
	{
		_parent = newParent;
		_parent->_childDar.PutElement(this);
		SetZPos(_zpos);

		if (_parent->Client())
			_parent->Client()->OnChildAdded((VPANEL)this);
	}
}

int VPanel::GetChildCount(void)
{
	return _childDar.GetCount();
}

VPanel *VPanel::GetChild(int index)
{
	return _childDar[index];
}

VPanel *VPanel::GetParent(void)
{
	return _parent;
}

void VPanel::SetZPos(int z)
{
	_zpos = z;

	if (_parent)
	{
		int childCount = _parent->GetChildCount();
		int i;

		for (i = 0; i < childCount; i++)
		{
			if (_parent->GetChild(i) == this)
				break;
		}

		if (i == childCount)
			return;

		while (1)
		{
			VPanel *prevChild = NULL, *nextChild = NULL;

			if (i > 0)
				prevChild = _parent->GetChild(i - 1);

			if (i < (childCount - 1))
				nextChild = _parent->GetChild(i + 1);

			if (i > 0 && prevChild && (prevChild->_zpos > _zpos))
			{
				_parent->_childDar.SetElementAt(prevChild, i);
				_parent->_childDar.SetElementAt(this, i - 1);
				i--;
			}
			else if (i < (childCount - 1) && nextChild && (nextChild->_zpos < _zpos))
			{
				_parent->_childDar.SetElementAt(nextChild, i);
				_parent->_childDar.SetElementAt(this, i + 1);
				i++;
			}
			else
			{
				break;
			}
		}
	}
}

int VPanel::GetZPos(void)
{
	return _zpos;
}

void VPanel::MoveToFront(void)
{
	surface()->MovePopupToFront((VPANEL)this);

	if (_parent)
	{
		_parent->_childDar.MoveElementToEnd(this);

		int i = _parent->_childDar.GetCount() - 2;

		while (i >= 0)
		{
			if (_parent->_childDar[i]->_zpos > _zpos)
			{
				_parent->_childDar.SetElementAt(_parent->_childDar[i], i + 1);
				_parent->_childDar.SetElementAt(this, i);

				i--;
			}
			else
			{
				break;
			}
		}
	}
}

void VPanel::MoveToBack(void)
{
	if (_parent)
	{
		_parent->_childDar.RemoveElement(this);
		_parent->_childDar.InsertElementAt(this, 0);

		int i = 1;

		while (i < _parent->_childDar.GetCount())
		{
			if (_parent->_childDar[i]->_zpos < _zpos)
			{
				_parent->_childDar.SetElementAt(_parent->_childDar[i], i - 1);
				_parent->_childDar.SetElementAt(this, i);
				i++;
			}
			else
			{
				break;
			}
		}
	}
}

bool VPanel::HasParent(VPanel *potentialParent)
{
	if (this == potentialParent)
		return true;

	if (_parent)
	{
		return _parent->HasParent(potentialParent);
	}

	return false;
}

SurfacePlat *VPanel::Plat(void)
{
	return _plat;
}

void VPanel::SetPlat(SurfacePlat *Plat)
{
	_plat = Plat;
}

bool VPanel::IsPopup(void)
{
	return _popup;
}

void VPanel::SetPopup(bool state)
{
	_popup = state;
}

bool VPanel::IsTopmostPopup(void) const
{
	return _isTopmostPopup;
}

void VPanel::SetTopmostPopup(bool bEnable)
{
	_isTopmostPopup = bEnable;
}

bool VPanel::IsFullyVisible(void)
{
	VPanel *panel = this;

	while (panel)
	{
		if (!panel->_visible)
			return false;

		panel = panel->_parent;
	}

	return true;
}

const char *VPanel::GetName(void)
{
	return Client()->GetName();
}

const char *VPanel::GetClassName(void)
{
	return Client()->GetClassName();
}

HScheme VPanel::GetScheme(void)
{
	return Client()->GetScheme();
}

void VPanel::SendMessage(KeyValues *params, VPANEL ifrompanel)
{
	Client()->OnMessage(params, ifrompanel);
}

void VPanel::SetKeyBoardInputEnabled(bool state)
{
	_kbInput = state;
}

void VPanel::SetMouseInputEnabled(bool state)
{
	_mouseInput = state;
}

bool VPanel::IsKeyBoardInputEnabled(void)
{
	return _kbInput;
}

bool VPanel::IsMouseInputEnabled(void)
{
	return _mouseInput;
}