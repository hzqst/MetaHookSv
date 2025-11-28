#include <stdio.h>
#include <string.h>

#include "Color.h"
#include <VGUI/IPanel.h>
#include <VGUI/IScheme.h>
#include <VGUI/ISurface.h>
#include <VGUI_controls/Controls.h>
#include "VPanel.h"
#include "KeyValues.h"
#include "Border.h"
#include "vgui_internal.h"

#include "tier0/memdbgon.h"

using namespace vgui;

Border::Border(void)
{
	_inset[0] = 0;
	_inset[1] = 0;
	_inset[2] = 0;
	_inset[3] = 0;
	_name = NULL;

	m_eBackgroundType = IBorder::BACKGROUND_FILLED;

	memset(_sides, 0, sizeof(_sides));
}

Border::~Border(void)
{
	delete [] _name;

	for (int i = 0; i < 4; i++)
		delete [] _sides[i].lines;
}

void Border::SetInset(int left, int top, int right, int bottom)
{
	_inset[SIDE_LEFT] = left;
	_inset[SIDE_TOP] = top;
	_inset[SIDE_RIGHT] = right;
	_inset[SIDE_BOTTOM] = bottom;
}

void Border::GetInset(int &left, int &top, int &right, int &bottom)
{
	left = _inset[SIDE_LEFT];
	top = _inset[SIDE_TOP];
	right = _inset[SIDE_RIGHT];
	bottom = _inset[SIDE_BOTTOM];
}

void Border::Paint(int x, int y, int wide, int tall)
{
	Paint(x, y, wide, tall, -1, 0, 0);
}

void Border::Paint(int x, int y, int wide, int tall, int breakSide, int breakStart, int breakEnd)
{
	int i;

	for (i = 0; i < _sides[SIDE_LEFT].count; i++)
	{
		line_t *line = &(_sides[SIDE_LEFT].lines[i]);
		surface()->DrawSetColor(line->col[0], line->col[1], line->col[2], line->col[3]);

		if (breakSide == SIDE_LEFT)
		{
			if (breakStart > 0)
				surface()->DrawFilledRect(x + i, y + line->startOffset, x + i + 1, y + breakStart);

			if (breakEnd < (tall - line->endOffset))
				surface()->DrawFilledRect(x + i, y + breakEnd + 1, x + i + 1, tall - line->endOffset);
		}
		else
			surface()->DrawFilledRect(x + i, y + line->startOffset, x + i + 1, tall - line->endOffset);
	}

	for (i = 0; i < _sides[SIDE_TOP].count; i++)
	{
		line_t *line = &(_sides[SIDE_TOP].lines[i]);
		surface()->DrawSetColor(line->col[0], line->col[1], line->col[2], line->col[3]);

		if (breakSide == SIDE_TOP)
		{
			if (breakStart > 0)
				surface()->DrawFilledRect(x + line->startOffset, y + i, x + breakStart, y + i + 1);

			if (breakEnd < (wide - line->endOffset))
				surface()->DrawFilledRect(x + breakEnd + 1, y + i, wide - line->endOffset, y + i + 1);
		}
		else
			surface()->DrawFilledRect(x + line->startOffset, y + i, wide - line->endOffset, y + i + 1);
	}

	for (i = 0; i < _sides[SIDE_RIGHT].count; i++)
	{
		line_t *line = &(_sides[SIDE_RIGHT].lines[i]);
		surface()->DrawSetColor(line->col[0], line->col[1], line->col[2], line->col[3]);
		surface()->DrawFilledRect(wide - (i + 1), y + line->startOffset, (wide - (i + 1)) + 1, tall - line->endOffset);
	}

	for (i = 0; i < _sides[SIDE_BOTTOM].count; i++)
	{
		line_t *line = &(_sides[SIDE_BOTTOM].lines[i]);
		surface()->DrawSetColor(line->col[0], line->col[1], line->col[2], line->col[3]);
		surface()->DrawFilledRect(x + line->startOffset, tall - (i + 1), wide - line->endOffset, (tall - (i + 1)) + 1);
	}

	m_bIsFristPaint = false;
}

void Border::Paint(VPANEL panel)
{
	int wide, tall;
	((VPanel *)panel)->GetSize(wide, tall);
	Paint(0, 0, wide, tall, -1, 0, 0);

	m_bIsFristPaint = false;
}

void Border::ApplySchemeSettings(IScheme *pScheme, KeyValues *inResourceData)
{
	const char *insetString = inResourceData->GetString("inset", "0 0 0 0");

	int left, top, right, bottom;
	GetInset(left, top, right, bottom);
	sscanf(insetString, "%d %d %d %d", &left, &top, &right, &bottom);
	SetInset(left, top, right, bottom);

	ParseSideSettings(SIDE_LEFT, inResourceData->FindKey("Left"), pScheme);
	ParseSideSettings(SIDE_TOP, inResourceData->FindKey("Top"), pScheme);
	ParseSideSettings(SIDE_RIGHT, inResourceData->FindKey("Right"), pScheme);
	ParseSideSettings(SIDE_BOTTOM, inResourceData->FindKey("Bottom"), pScheme);

	m_eBackgroundType = (backgroundtype_e)inResourceData->GetInt("backgroundtype");
	m_bIsFristPaint = true;
}

void Border::ParseSideSettings(int side_index, KeyValues *inResourceData, IScheme *pScheme)
{
	if (!inResourceData)
		return;

	int count = 0;
	KeyValues *kv = nullptr;

	for (kv = inResourceData->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
		count++;

	_sides[side_index].count = count;
	_sides[side_index].lines = new line_t[count];

	int index = 0;

	for (kv = inResourceData->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
	{
		line_t *line = &(_sides[side_index].lines[index]);

		const char *col = kv->GetString("color", NULL);
		line->col = pScheme->GetColor(col, Color(0, 0, 0, 0));
		col = kv->GetString("offset", NULL);

		int Start = 0, end = 0;

		if (col)
			sscanf(col, "%d %d", &Start, &end);

		line->startOffset = Start;
		line->endOffset = end;

		index++;
	}
}

const char *Border::GetName(void)
{
	if (_name)
		return _name;

	return "";
}

void Border::SetName(const char *name)
{
	if (_name)
		delete [] _name;

	int len = Q_strlen(name) + 1;
	_name = new char[len];
	Q_strncpy(_name, name, len);
}

IBorder::backgroundtype_e Border::GetBackgroundType(void)
{
	return m_eBackgroundType;
}