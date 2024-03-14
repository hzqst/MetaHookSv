#ifndef IENGINE_H
#define IENGINE_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class IEngine : public IBaseInterface
{
public:
	enum
	{
		QUIT_NOTQUITTING = 0,
		QUIT_TODESKTOP,
		QUIT_RESTART
	};

	virtual	bool	Load( bool dedicated, char *basedir, char *cmdline ) = 0;
	virtual void	Unload( void ) = 0;
	virtual	void	SetState( int iState ) = 0;
	virtual int		GetState( void ) = 0;
	virtual	void	SetSubState( int iSubState ) = 0;
	virtual int		GetSubState( void ) = 0;
	virtual int		Frame( void ) = 0;
	virtual double	GetFrameTime( void ) = 0;
	virtual double	GetCurTime( void ) = 0;
	virtual void	TrapKey_Event( int key, bool down ) = 0;
	virtual void	TrapMouse_Event( int buttons, bool down ) = 0;
	virtual void	StartTrapMode( void ) = 0;
	virtual bool	IsTrapping( void ) = 0;
	virtual bool	CheckDoneTrapping( int& buttons, int& key ) = 0;
	virtual int		GetQuitting( void ) = 0;
	virtual void	SetQuitting( int quittype ) = 0;

};

#endif //IENGINE_H