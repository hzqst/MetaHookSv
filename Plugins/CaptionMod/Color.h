#ifndef _COLOR_H
#define _COLOR_H

class Color
{
public:
	Color(void) { SetColor(0, 0, 0, 0); }
	Color(int r, int g, int b) { SetColor(r, g, b, 0); }
	Color(int r, int g, int b, int a) { SetColor(r, g, b, a); }

public:
	void SetColor(int r, int g, int b, int a)
	{
		_color[0] = (unsigned char)r;
		_color[1] = (unsigned char)g;
		_color[2] = (unsigned char)b;
		_color[3] = (unsigned char)a;
	}

	void GetColor(int &r, int &g, int &b, int &a) const
	{
		r = _color[0];
		g = _color[1];
		b = _color[2];
		a = _color[3];
	}

public:
	int r(void) { return _color[0]; }
	int g(void) { return _color[1]; }
	int b(void) { return _color[2]; }
	int a(void) { return _color[3]; }

public:
	unsigned char &operator[](int index)
	{
		return _color[index];
	}

	bool operator == (Color &rhs) const
	{
		return (_color[0] == rhs._color[0] && _color[1] == rhs._color[1] && _color[2] == rhs._color[2] && _color[3] == rhs._color[3]);
	}

	bool operator != (Color &rhs) const
	{
		return !(operator == (rhs));
	}

private:
	unsigned char _color[4];
};

#endif