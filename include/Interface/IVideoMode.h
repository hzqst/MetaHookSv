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
	virtual const char* GetName() const;
	virtual void Init();
	virtual void Shutdown();
	virtual bool AddMode(int width, int height, int bpp);
	virtual const videomode_t* GetCurrentMode() const;
	virtual const videomode_t* GetMode(int num) const;
	virtual int GetModeCount() const;
	virtual bool IsWindowedMode() const;
	virtual bool GetInitialized() const;
	virtual void SetInitialized(bool init);
	virtual void UpdateWindowPosition();
	virtual void unk();//not implemented
	virtual void RestoreVideo();
	virtual void ReleaseVideo();
	virtual void Destroy();
	virtual int GetBitsPerPixel();
	virtual void Minimize();
	virtual void RestoreFromMinimize();
};

class IVideoMode_HL25
{
public:
	virtual const char* GetName() const;
	virtual void Init();
	virtual void Startup();//PlayStartupVideo or DrawStartupGraphic
	virtual void Shutdown();
	virtual bool AddMode(int width, int height, int bpp);
	virtual const videomode_t* GetCurrentMode() const;
	virtual const videomode_t* GetMode(int num) const;
	virtual int GetModeCount() const;
	virtual bool IsWindowedMode() const;
	virtual bool GetInitialized() const;
	virtual void SetInitialized(bool init);
	virtual void UpdateWindowPosition();
	virtual void unk();//not implemented
	virtual void RestoreVideo();
	virtual void ReleaseVideo();
	virtual void Destroy();
	virtual int GetBitsPerPixel();
	virtual void Minimize();
	virtual void RestoreFromMinimize();
};

#endif
