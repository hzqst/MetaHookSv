#ifndef BITMAP_H
#define BITMAP_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/IImage.h>
#include <Color.h>

namespace vgui
{

typedef unsigned long HTexture;

class Bitmap : public IImage
{
public:
	Bitmap(const char *filename, bool hardwareFiltered);
	~Bitmap(void);

public:
	virtual void Paint(void);
	virtual void GetSize(int &wide, int &tall);
	virtual void GetContentSize(int &wide, int &tall);
	virtual void SetSize(int x, int y);
	virtual void SetPos(int x, int y);
	virtual void SetColor(Color col);

public:
	void ForceUpload(void);
	HTexture GetID(void);
	const char *GetName(void);
	bool IsValid(void) { return _valid; }

private:
	HTexture _id;
	bool _uploaded;
	bool _valid;
	char *_filename;
	int _pos[2];
	Color _color;
	bool _filtered;
	int _wide,_tall;
	bool _bProcedural;
};
}

#endif
