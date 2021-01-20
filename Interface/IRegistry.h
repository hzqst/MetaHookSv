#ifndef IREGISTRY_H
#define IREGISTRY_H

#ifdef _WIN32
#pragma once
#endif

class IRegistry
{
public:
	virtual void Init(void) = 0;
	virtual void Shutdown(void) = 0;
	virtual int ReadInt(const char *key, int defaultValue = 0) = 0;
	virtual void WriteInt(const char *key, int value) = 0;
	virtual const char *ReadString(const char *key, const char *defaultValue = NULL) = 0;
	virtual void WriteString(const char *key, const char *value) = 0;
};

extern IRegistry *registry;

#endif