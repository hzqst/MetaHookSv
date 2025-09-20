#pragma once

#include <deque>

class CPMBRingBuffer
{
private:
	GLuint m_RingVBO{};
	//void* m_MappedPtr;
	size_t m_BufferSize{};

	// 简化的 Head/Tail 指针机制
	size_t m_Head;          // 写入指针
	size_t m_Tail;          // 释放指针
	size_t m_UsedSize;      // 已使用空间
	size_t m_CurrFrameSize; // 当前帧大小

	// 已完成帧的fence信息
	struct FrameHeadAttribs
	{
		GLsync fence;      // OpenGL fence对象
		size_t offset;     // 帧在缓冲区中的偏移
		size_t size;       // 帧的大小

		FrameHeadAttribs(GLsync f, size_t off, size_t sz)
			: fence(f), offset(off), size(sz) {}
	};

	std::deque<FrameHeadAttribs> m_CompletedFrames;

public:
	struct Allocation
	{
		void* ptr{};
		size_t offset{};
		size_t size{};
		bool valid{};
	};

	bool Initialize(size_t bufferSize); // 8MB default
	void Shutdown();
	bool Allocate(size_t size, size_t alignment, CPMBRingBuffer::Allocation& allocation);
	void Commit(CPMBRingBuffer::Allocation& allocation);
	void BeginFrame();
	void EndFrame();
	void Reset();
	GLuint GetVBO() const { return m_RingVBO; }

	// 调试和状态查询
	bool IsEmpty() const { return m_UsedSize == 0; }
	bool IsFull() const { return m_UsedSize == m_BufferSize; }
	size_t GetUsedSize() const { return m_UsedSize; }

private:
	void ReleaseCompletedFrames();
	static bool IsPowerOfTwo(size_t value) { return value && !(value & (value - 1)); }
	static size_t AlignUp(size_t value, size_t alignment) { return (value + alignment - 1) & ~(alignment - 1); }
};