#include "gl_local.h"

#include <deque>

class CPMBRingBuffer : public IPMBRingBuffer
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
	bool m_InFrame{};
	bool m_LoggedOutOfFrameAllocate{};

	struct FrameHeadAttribs
	{
		GLsync fence;      // OpenGL fence
		size_t offset;     // offset in current frame
		size_t size;       // frame size

		FrameHeadAttribs(GLsync f, size_t off, size_t sz)
			: fence(f), offset(off), size(sz) {
		}
	};

	std::deque<FrameHeadAttribs> m_CompletedFrames;
	std::string m_BufferName;

public:
	
	CPMBRingBuffer(const char* name, size_t bufferSize, GLenum bufferTarget)
	{
		m_BufferName = name;
		m_BufferSize = bufferSize;
		m_Head = 0;
		m_Tail = 0;
		m_UsedSize = 0;
		m_CurrFrameSize = 0;
		m_FrameStartOffset = 0;
		m_InFrame = false;
		m_LoggedOutOfFrameAllocate = false;

		m_CompletedFrames.clear();

		m_GLBufferTarget = bufferTarget;
		m_hGLBufferObject = GL_GenBuffer();

		GL_BindVAO(0);

		glBindBuffer(m_GLBufferTarget, m_hGLBufferObject);

		glBufferStorage(m_GLBufferTarget, m_BufferSize, nullptr,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

		m_MappedPtr = glMapBufferRange(m_GLBufferTarget, 0, m_BufferSize,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

		glBindBuffer(m_GLBufferTarget, 0);

		if (glObjectLabel)
		{
			glObjectLabel(GL_BUFFER, m_hGLBufferObject, -1, m_BufferName.c_str());
		}
	}

	~CPMBRingBuffer()
	{
		for (auto& frame : m_CompletedFrames)
		{
			if (frame.fence)
			{
				glDeleteSync(frame.fence);
			}
		}
		m_CompletedFrames.clear();

		if (m_MappedPtr)
		{
			GL_BindVAO(0);

			if (m_hGLBufferObject)
			{
				glBindBuffer(m_GLBufferTarget, m_hGLBufferObject);
				glUnmapBuffer(m_GLBufferTarget);
				glBindBuffer(m_GLBufferTarget, 0);
			}

			m_MappedPtr = nullptr;
		}

		if (m_hGLBufferObject)
		{
			GL_DeleteBuffer(m_hGLBufferObject);
			m_hGLBufferObject = 0;
		}

		m_GLBufferTarget = 0;
		m_Head = 0;
		m_Tail = 0;
		m_UsedSize = 0;
		m_CurrFrameSize = 0;
		m_FrameStartOffset = 0;
		m_InFrame = false;
	}

	void Destroy()  override
	{
		delete this;
	}

	bool Allocate(size_t size, CPMBRingBufferAllocation& allocation) override;
	void BeginFrame() override;
	void EndFrame() override;
	
	GLuint GetGLBufferObject() const override { return m_hGLBufferObject; }

	bool IsEmpty() const override { return m_UsedSize == 0; }
	bool IsFull() const override { return m_UsedSize == m_BufferSize; }
	size_t GetUsedSize() const override { return m_UsedSize; }

private:
	void ReleaseCompletedFrames();
	bool WaitForAvailableSpace(size_t size);
	bool WaitForFrameIfOverlapping(size_t allocStart, size_t allocSize);
	bool WaitAndReleaseFrame(std::deque<FrameHeadAttribs>::iterator it, GLuint64 timeout, const char* reason);
	void ReleaseFrame(std::deque<FrameHeadAttribs>::iterator it);
	void ResetAfterGpuDrain();
	bool HasAvailableSpace(size_t size) const;
	size_t WrapOffset(size_t offset) const;
	bool DoRangesOverlap(size_t start1, size_t size1, size_t start2, size_t size2) const;
	static bool IsPowerOfTwo(size_t value) { return value && !(value & (value - 1)); }
};

bool CPMBRingBuffer::Allocate(size_t size, CPMBRingBufferAllocation& allocation)
{
	allocation = {};

	if (size == 0)
		return false;

	if (!m_InFrame)
	{
		if (!m_LoggedOutOfFrameAllocate)
		{
			gEngfuncs.Con_Printf("[%s] Warning: Allocate called outside frame, requested=%u used=%u buffer=%u curr=%u completed=%u head=%u tail=%u\n",
				m_BufferName.c_str(),
				(unsigned int)size,
				(unsigned int)m_UsedSize,
				(unsigned int)m_BufferSize,
				(unsigned int)m_CurrFrameSize,
				(unsigned int)m_CompletedFrames.size(),
				(unsigned int)m_Head,
				(unsigned int)m_Tail);
			m_LoggedOutOfFrameAllocate = true;
		}

		return false;
	}

	if (size > m_BufferSize)
	{
		return false;
	}

	ReleaseCompletedFrames();

	while (!HasAvailableSpace(size))
	{
		if (!WaitForAvailableSpace(size))
		{
			return false;
		}
	}

	for (;;)
	{
		size_t alignedHead = m_Head;

		if (m_Head >= m_Tail)
		{
			// [----Tail####Head----]
			if (alignedHead + size <= m_BufferSize)
			{
				if (!WaitForFrameIfOverlapping(alignedHead, size))
				{
					return false;
				}

				allocation.ptr = (char*)m_MappedPtr + alignedHead;
				allocation.offset = alignedHead;
				allocation.size = size;
				allocation.valid = true;

				size_t adjustedSize = size + (alignedHead - m_Head);
				m_Head = alignedHead + size;
				m_UsedSize += adjustedSize;
				m_CurrFrameSize += adjustedSize;
				return true;
			}
			else if (size <= m_Tail)
			{
				size_t wastedSpace = m_BufferSize - m_Head;

				if (!WaitForFrameIfOverlapping(0, size))
				{
					return false;
				}

				allocation.ptr = (char*)m_MappedPtr;
				allocation.offset = 0;
				allocation.size = size;
				allocation.valid = true;

				m_Head = size;
				m_UsedSize += wastedSpace + size;
				m_CurrFrameSize += wastedSpace + size;
				return true;
			}
		}
		else
		{
			// [####Head----Tail####]
			if (alignedHead + size <= m_Tail)
			{
				if (!WaitForFrameIfOverlapping(alignedHead, size))
				{
					return false;
				}

				allocation.ptr = (char*)m_MappedPtr + alignedHead;
				allocation.offset = alignedHead;
				allocation.size = size;
				allocation.valid = true;

				size_t adjustedSize = size + (alignedHead - m_Head);
				m_Head = alignedHead + size;
				m_UsedSize += adjustedSize;
				m_CurrFrameSize += adjustedSize;
				return true;
			}
			else if (m_Tail + size <= m_BufferSize)
			{
				size_t alignedTail = m_Tail;
				if (alignedTail + size <= m_BufferSize)
				{
					size_t wastedSpace = m_Tail - m_Head;

					if (!WaitForFrameIfOverlapping(alignedTail, size))
					{
						return false;
					}

					allocation.ptr = (char*)m_MappedPtr + alignedTail;
					allocation.offset = alignedTail;
					allocation.size = size;
					allocation.valid = true;

					m_Head = alignedTail + size;
					m_UsedSize += wastedSpace + size + (alignedTail - m_Tail);
					m_CurrFrameSize += wastedSpace + size + (alignedTail - m_Tail);
					return true;
				}
			}
		}

		if (m_CompletedFrames.empty() || !WaitAndReleaseFrame(m_CompletedFrames.begin(), GL_TIMEOUT_IGNORED, "fragmented allocation"))
		{
			return false;
		}
	}
}

void CPMBRingBuffer::BeginFrame()
{
	ReleaseCompletedFrames();

	m_CurrFrameSize = 0;
	m_FrameStartOffset = m_Head;
	m_InFrame = true;
}

void CPMBRingBuffer::EndFrame()
{
	if (m_CurrFrameSize > 0)
	{
		GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		if (fence)
		{
			m_CompletedFrames.emplace_back(fence, m_FrameStartOffset, m_CurrFrameSize);
			m_CurrFrameSize = 0;
			m_InFrame = false;
			return;
		}

		GLenum error = glGetError();
		gEngfuncs.Con_Printf("[%s] Warning: glFenceSync failed, glGetError=0x%X curr=%u used=%u buffer=%u completed=%u. Resetting ring buffer after glFinish.\n",
			m_BufferName.c_str(),
			(unsigned int)error,
			(unsigned int)m_CurrFrameSize,
			(unsigned int)m_UsedSize,
			(unsigned int)m_BufferSize,
			(unsigned int)m_CompletedFrames.size());

		glFinish();
		ResetAfterGpuDrain();
	}

	m_InFrame = false;
}

void CPMBRingBuffer::ReleaseCompletedFrames()
{
	while (!m_CompletedFrames.empty())
	{
		if (!WaitAndReleaseFrame(m_CompletedFrames.begin(), 0, "completed frame release"))
		{
			break;
		}
	}

	if (IsEmpty())
	{
		m_CompletedFrames.clear();
		m_Head = 0;
		m_Tail = 0;
	}
}

bool CPMBRingBuffer::WaitForAvailableSpace(size_t size)
{
	while (!HasAvailableSpace(size))
	{
		if (m_CompletedFrames.empty())
		{
			return false;
		}

		if (!WaitAndReleaseFrame(m_CompletedFrames.begin(), GL_TIMEOUT_IGNORED, "available space"))
		{
			return false;
		}
	}

	return true;
}

bool CPMBRingBuffer::WaitAndReleaseFrame(std::deque<FrameHeadAttribs>::iterator it, GLuint64 timeout, const char* reason)
{
	GLenum result = glClientWaitSync(it->fence, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
	if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED)
	{
		ReleaseFrame(it);
		return true;
	}

	if (result == GL_WAIT_FAILED)
	{
		gEngfuncs.Con_Printf("[%s] Warning: glClientWaitSync failed while waiting for %s (result=%u)\n",
			m_BufferName.c_str(), reason, (unsigned int)result);
	}

	return false;
}

void CPMBRingBuffer::ReleaseFrame(std::deque<FrameHeadAttribs>::iterator it)
{
	size_t frameEnd = WrapOffset(it->offset + it->size);

	if (it->size <= m_UsedSize)
	{
		m_UsedSize -= it->size;
	}
	else
	{
		m_UsedSize = 0;
	}

	if (it == m_CompletedFrames.begin())
	{
		m_Tail = frameEnd;
	}

	if (it->fence)
	{
		glDeleteSync(it->fence);
	}

	m_CompletedFrames.erase(it);
}

void CPMBRingBuffer::ResetAfterGpuDrain()
{
	for (auto& frame : m_CompletedFrames)
	{
		if (frame.fence)
		{
			glDeleteSync(frame.fence);
		}
	}
	m_CompletedFrames.clear();

	m_Head = 0;
	m_Tail = 0;
	m_UsedSize = 0;
	m_CurrFrameSize = 0;
	m_FrameStartOffset = 0;
}

bool CPMBRingBuffer::HasAvailableSpace(size_t size) const
{
	return size <= m_BufferSize && m_UsedSize <= m_BufferSize - size;
}

size_t CPMBRingBuffer::WrapOffset(size_t offset) const
{
	if (m_BufferSize == 0)
	{
		return 0;
	}

	return offset % m_BufferSize;
}

// 检测两个区域是否在 ring buffer 中重叠
// 需要考虑 ring buffer 的环形特性
bool CPMBRingBuffer::DoRangesOverlap(size_t start1, size_t size1, size_t start2, size_t size2) const
{
	if (size1 == 0 || size2 == 0)
		return false;

	size_t end1 = start1 + size1;
	size_t end2 = start2 + size2;

	// 情况1: 两个范围都不跨越 buffer 边界
	if (end1 <= m_BufferSize && end2 <= m_BufferSize)
	{
		// 标准区间重叠检测: [start1, end1) 和 [start2, end2) 是否重叠
		return !(end1 <= start2 || end2 <= start1);
	}

	// 情况2: range1 跨越边界 (wrap around)
	if (end1 > m_BufferSize)
	{
		// range1 = [start1, bufferSize) ∪ [0, end1 - bufferSize)
		size_t wrapped_end1 = end1 - m_BufferSize;

		if (end2 <= m_BufferSize)
		{
			// range2 不跨界: 检查是否与 range1 的任一部分重叠
			// 部分1: [start1, bufferSize) vs [start2, end2)
			// 部分2: [0, wrapped_end1) vs [start2, end2)
			return (start2 < m_BufferSize && end2 > start1) || (start2 < wrapped_end1);
		}
		else
		{
			// range2 也跨界: 必然重叠 (因为都在使用头尾两段)
			return true;
		}
	}

	// 情况3: range2 跨越边界但 range1 不跨界
	if (end2 > m_BufferSize)
	{
		// range2 = [start2, bufferSize) ∪ [0, end2 - bufferSize)
		size_t wrapped_end2 = end2 - m_BufferSize;

		// range1 不跨界: 检查是否与 range2 的任一部分重叠
		return (start1 < m_BufferSize && end1 > start2) || (start1 < wrapped_end2);
	}

	return false;
}

// 如果新分配区域与正在被 GPU 使用的帧重叠,等待该帧完成
bool CPMBRingBuffer::WaitForFrameIfOverlapping(size_t allocStart, size_t allocSize)
{
	// 遍历所有未完成的帧,检查是否与新分配区域重叠
	auto it = m_CompletedFrames.begin();
	while (it != m_CompletedFrames.end())
	{
		const auto& frame = *it;

		// 检测新分配区域 [allocStart, allocStart+allocSize) 是否与该帧 [frame.offset, frame.offset+frame.size) 重叠
		if (DoRangesOverlap(allocStart, allocSize, frame.offset, frame.size))
		{
			if (!WaitAndReleaseFrame(it, GL_TIMEOUT_IGNORED, "overlapping frame"))
			{
				return false;
			}

			it = m_CompletedFrames.begin();
			continue;
		}

		++it;
	}

	return true;
}

IPMBRingBuffer* GL_CreatePMBRingBuffer(const char* name, size_t bufferSize, GLenum bufferTarget)
{
	return new CPMBRingBuffer(name, bufferSize, bufferTarget);
}
