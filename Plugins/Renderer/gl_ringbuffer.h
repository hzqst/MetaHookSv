#pragma once

#include <deque>

class CPMBRingBuffer
{
private:
	GLuint m_hGLBufferObject{};
	GLenum m_GLBufferTarget{};
	void* m_MappedPtr{};
	size_t m_BufferSize{};

	size_t m_Head{};
	size_t m_Tail{};
	size_t m_UsedSize{};
	size_t m_CurrFrameSize{}; 
	size_t m_FrameStartOffset{};

	struct FrameHeadAttribs
	{
		GLsync fence;      // OpenGL fence
		size_t offset;     // offset in current frame
		size_t size;       // frame size

		FrameHeadAttribs(GLsync f, size_t off, size_t sz)
			: fence(f), offset(off), size(sz) {}
	};

	std::deque<FrameHeadAttribs> m_CompletedFrames;
	std::string m_BufferName;
public:
	struct Allocation
	{
		void* ptr{};
		size_t offset{};
		size_t size{};
		bool valid{};
	};

	bool Initialize(const char* name, size_t bufferSize, GLenum bufferTarget);
	void Shutdown();
	bool Allocate(size_t size, size_t alignment, CPMBRingBuffer::Allocation& allocation);
	void BeginFrame();
	void EndFrame();
	void Reset();
	GLuint GetGLBufferObject() const { return m_hGLBufferObject; }

	bool IsEmpty() const { return m_UsedSize == 0; }
	bool IsFull() const { return m_UsedSize == m_BufferSize; }
	size_t GetUsedSize() const { return m_UsedSize; }

private:
	void ReleaseCompletedFrames();
	void WaitForFrameIfOverlapping(size_t allocStart, size_t allocSize);
	bool DoRangesOverlap(size_t start1, size_t size1, size_t start2, size_t size2) const;
	static bool IsPowerOfTwo(size_t value) { return value && !(value & (value - 1)); }
	static size_t AlignUp(size_t value, size_t alignment) { return (value + alignment - 1) & ~(alignment - 1); }
};