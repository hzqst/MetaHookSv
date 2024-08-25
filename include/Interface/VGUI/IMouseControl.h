//========= Copyright ?1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef IMOUSECONTROL_H
#define IMOUSECONTROL_H
#ifdef _WIN32
#pragma once
#endif

class IMouseControl
{
public:
	virtual bool VGUI2MouseControl( void ) = 0;
	virtual void SetVGUI2MouseControl( bool state ) = 0;
};

#endif // IMOUSECONTROL_H
