#ifndef IVIDEOMODE_H
#define IVIDEOMODE_H

#ifdef _WIN32
#pragma once
#endif

typedef struct videomode_s
{
	int width;
	int height;
	int bpp;
}videomode_t;

class IVideoMode
{
public:
	virtual const char* GetName();
	virtual void Init();
	virtual void Shutdown();
	virtual bool AddMode(int width, int height, int bpp);
	virtual videomode_t* GetCurrentMode();
	virtual videomode_t* GetMode(int num);
	virtual int GetModeCount();
	virtual bool IsWindowedMode();
	virtual bool GetInitialized();
	virtual void SetInitialized(bool init);
	virtual void UpdateWindowPosition();
	virtual void FlipScreen();
	virtual void RestoreVideo();
	virtual void ReleaseVideo();
	virtual void Destroy();
	virtual int GetBitsPerPixel();
};

class IVideoMode_HL25
{
public:
	virtual const char* GetName();
	virtual void Init();
	virtual void Shutdown();
	virtual void unk();
	virtual bool AddMode(int width, int height, int bpp);
	virtual videomode_t* GetCurrentMode();
	virtual videomode_t* GetMode(int num);
	virtual int GetModeCount();
	virtual bool IsWindowedMode();
	virtual bool GetInitialized();
	virtual void SetInitialized(bool init);
	virtual void UpdateWindowPosition();
	virtual void FlipScreen();
	virtual void RestoreVideo();
	virtual void ReleaseVideo();
	virtual void Destroy();
	virtual int GetBitsPerPixel();
};

#endif
