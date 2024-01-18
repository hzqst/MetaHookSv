//========= Copyright ?1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VGUIVERTEX_H
#define VGUIVERTEX_H

#ifdef _WIN32
#pragma once
#endif

namespace vgui
{
class VGuiVertex
{
public:
	VGuiVertex()
	{
		SetVertex(0,0,0,0);
	}

	VGuiVertex( int x, int y, float u, float v )
	{
		SetVertex( x, y, u, v );
	}

	void SetX( int i ) { x = i; }
	void SetY( int i ) { y = i; }
	void SetU( float f ) { u = f; }
	void SetV( float f ) { v = f; }
	void SetVertex( int x, int y )
	{
		SetX( x );
		SetY( y );
	}
	void SetVertex( int x, int y, float u, float v )
	{
		SetX( x );
		SetY( y );
		SetU( u );
		SetV( v );
	}

	int GetX() { return x; }
	int GetY() { return y; }
	float GetU() { return u; }
	float GetV() { return v; }
	void GetVertex( int &x, int &y, float &u, float &v )
	{
		x = GetX();
		y = GetY();
		u = GetU();
		v = GetV();
	}

	bool operator==( VGuiVertex &from )
	{
		return (from.x == x && from.y == y && from.u == u && from.v == v);
	}

private:
	int		x, y;
	float	u, v;
};
}

#endif // VGUIVERTEX_H
