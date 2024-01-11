#include <vgui/ISurface.h>
#include "bitmap.h"
#include "vgui_internal.h"
#include "FileSystem.h"
#include "UtlBuffer.h"
#include <tier0/dbg.h>

#include <vgui_controls/Controls.h>

#include "tier0/memdbgon.h"

using namespace vgui;

Bitmap::Bitmap(const char *filename, bool hardwareFiltered)
{
	_filtered = hardwareFiltered;

	int size = strlen(filename) + 1;
	_filename = (char *)malloc(size);
	Assert(_filename);

	Q_snprintf(_filename, size, "%s", filename);

	_bProcedural = false;
	_bAdditive = false;

	if (Q_stristr(filename, ".pic"))
		_bProcedural = true;

	_id = 0;
	_uploaded = false;
	_color = Color(255, 255, 255, 255);
	_pos[0] = _pos[1] = 0;
	_valid = true;
	_wide = 0;
	_tall = 0;

	ForceUpload();
}

Bitmap::~Bitmap(void)
{
	if (_filename)
		free(_filename);
}

void Bitmap::GetSize(int &wide, int &tall)
{
	wide = 0;
	tall = 0;

	if (!_valid)
		return;

	if (0 == _wide && 0 == _tall)
		surface()->DrawGetTextureSize(_id, _wide, _tall);

	wide = _wide;
	tall = _tall;
}

void Bitmap::GetContentSize(int &wide, int &tall)
{
	GetSize(wide, tall);
}

void Bitmap::SetSize(int x, int y)
{
	_wide = x;
	_tall = y;
}

void Bitmap::SetPos(int x, int y)
{
	_pos[0] = x;
	_pos[1] = y;
}

void Bitmap::SetColor(Color col)
{
	_color = col;
}

const char *Bitmap::GetName(void)
{
	return _filename;
}

void Bitmap::Paint(void)
{
	if (!_valid)
		return;

	if (!_id)
		_id = surface()->CreateNewTextureID();

	if (!_uploaded)
		ForceUpload();

	surface()->DrawSetColor(_color[0], _color[1], _color[2], _color[3]);
	surface()->DrawSetTexture(_id);

	if (_wide == 0)
		GetSize(_wide, _tall);

	if (_bAdditive)
	{
		surface()->DrawTexturedRectAdd(_pos[0], _pos[1], _pos[0] + _wide, _pos[1] + _tall);
	}
	else
	{
		surface()->DrawTexturedRect(_pos[0], _pos[1], _pos[0] + _wide, _pos[1] + _tall);
	}
}

void Bitmap::ForceUpload(void)
{
	if (!_valid || _uploaded)
		return;

	if (!_id)
		_id = surface()->CreateNewTextureID(_bProcedural);

	if (!_bProcedural)
		surface()->DrawSetTextureFile(_id, _filename, _filtered, false);

	_uploaded = true;
	_valid = surface()->IsTextureIDValid(_id);
}

HTexture Bitmap::GetID(void)
{
	return _id;
}

void Bitmap::Destroy(void)
{
	delete this;
}

void Bitmap::SetAdditive(bool bIsAdditive)
{
	_bAdditive = bIsAdditive;
}