#pragma once

#include <interface.h>
#include <stdint.h>

class CPMBRingBufferAllocation
{
public:
	void* ptr{};
	size_t offset{};
	size_t size{};
	bool valid{};
};

class IPMBRingBuffer : public IBaseInterface
{
public:
	/*
		Must be called on game shutting down to free memory and OpenGL resources.
	*/
	virtual void Destroy() = 0;

	/*
		Allocate "size" bytes of memory from ring buffer, the result returns from "allocation"
	*/
	virtual bool Allocate(size_t size, CPMBRingBufferAllocation& allocation) = 0;
	
	/*
		Must be called on frame start
	*/
	virtual void BeginFrame() = 0;

	/*
		Must be called on frame end, before present
	*/
	virtual void EndFrame() = 0;

	/*
		Get corresponding OpenGL Buffer Object (VBO, EBO or whatever)
	*/
	virtual GLuint GetGLBufferObject() const = 0;

	virtual bool IsEmpty() const = 0;
	virtual bool IsFull() const = 0;
	virtual size_t GetUsedSize() const = 0;
};

IPMBRingBuffer* GL_CreatePMBRingBuffer(const char* name, size_t bufferSize, GLenum bufferTarget);