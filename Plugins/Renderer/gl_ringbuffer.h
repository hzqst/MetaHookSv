#pragma once

class CPMBRingBuffer
{
private:
	GLuint m_RingVBO;
	void* m_MappedPtr;
	size_t m_BufferSize;
	size_t m_WriteOffset;

	// 跟踪帧的使用情况
	struct FrameInfo
	{
		size_t startOffset;
		size_t endOffset;
		GLsync fence;
		bool active;
	};

	static const int MAX_FRAMES_IN_FLIGHT = 3;
	FrameInfo m_Frames[MAX_FRAMES_IN_FLIGHT];
	int m_CurrentFrame;

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
	void BeginFrame();
	void EndFrame();
	void Reset();
	GLuint GetVBO() const { return m_RingVBO; }

private:
	bool CanAllocateAt(size_t offset, size_t size);
	void CleanupCompletedFrames();
};