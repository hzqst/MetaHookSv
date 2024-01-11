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

//Best compatibility for HL25 and non-HL25 engine
class Bitmap : public IImage_HL25
{
public:
	Bitmap(const char *filename, bool hardwareFiltered);
	~Bitmap(void);

public:
	void Paint(void) override;
	void GetSize(int &wide, int &tall) override;
	void GetContentSize(int &wide, int &tall) override;
	void SetSize(int x, int y) override;
	void SetPos(int x, int y) override;
	void SetColor(Color col) override;
	void Destroy(void) override;
	void SetAdditive(bool bIsAdditive) override;
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
	bool _bAdditive;
};
}

#endif
