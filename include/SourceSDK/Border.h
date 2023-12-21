#ifndef BORDER_H
#define BORDER_H

#include "vgui/IBorder.h"

class Color;

namespace vgui
{

class Border : public IBorder
{
public:
	Border(void);
	~Border(void);

	virtual void Paint(VPANEL panel);
	virtual void Paint(int x0, int y0, int x1, int y1);
	virtual void Paint(int x0, int y0, int x1, int y1, int breakSide, int breakStart, int breakStop);
	virtual void SetInset(int left, int top, int right, int bottom);
	virtual void GetInset(int &left, int &top, int &right, int &bottom);

	virtual void ApplySchemeSettings(IScheme *pScheme, KeyValues *inResourceData);

	virtual const char *GetName(void);
	virtual void SetName(const char *name);
	virtual backgroundtype_e GetBackgroundType(void);
	virtual bool PaintFirst(void) { return m_bIsFristPaint; }

protected:
	int _inset[4];

private:
	Border(Border&);

private:
	void ParseSideSettings(int side_index, KeyValues *inResourceData, IScheme *pScheme);

private:
	char *_name;

	struct line_t
	{
		Color col;
		int startOffset;
		int endOffset;
	};

	struct side_t
	{
		int count;
		line_t *lines;
	};

	side_t _sides[4];
	backgroundtype_e m_eBackgroundType;
	bool m_bIsFristPaint;

private:
	friend class VPanel;
};
}

#endif