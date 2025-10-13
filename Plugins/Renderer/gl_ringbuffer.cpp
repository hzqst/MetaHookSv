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

		if (m_GLBufferTarget)
		{
			GL_DeleteBuffer(m_GLBufferTarget);
			m_GLBufferTarget = 0;
		}

		m_Head = 0;
		m_Tail = 0;
		m_UsedSize = 0;
		m_CurrFrameSize = 0;
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
	void WaitForFrameIfOverlapping(size_t allocStart, size_t allocSize);
	bool DoRangesOverlap(size_t start1, size_t size1, size_t start2, size_t size2) const;
	static bool IsPowerOfTwo(size_t value) { return value && !(value & (value - 1)); }
};

bool CPMBRingBuffer::Allocate(size_t size, CPMBRingBufferAllocation& allocation)
{
	if (size == 0)
		return false;

	if (m_UsedSize + size > m_BufferSize)
	{
		return false;
	}

	size_t alignedHead = m_Head;

	if (m_Head >= m_Tail)
	{
		// [----Tail####Head----]
		if (alignedHead + size <= m_BufferSize)
		{
		
			WaitForFrameIfOverlapping(alignedHead, size);

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

			WaitForFrameIfOverlapping(0, size);

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
			WaitForFrameIfOverlapping(alignedHead, size);

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

				WaitForFrameIfOverlapping(alignedTail, size);

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

	return false;
}

void CPMBRingBuffer::BeginFrame()
{
	ReleaseCompletedFrames();

	m_CurrFrameSize = 0;
	m_FrameStartOffset = m_Head;
}

void CPMBRingBuffer::EndFrame()
{
	if (m_CurrFrameSize > 0)
	{
		GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		if (fence)
		{
			m_CompletedFrames.emplace_back(fence, m_FrameStartOffset, m_CurrFrameSize);
		}

		//gEngfuncs.Con_DPrintf("%s: %d bytes used, from %d.\n", m_BufferName.c_str(), m_CurrFrameSize, m_FrameStartOffset);

		m_CurrFrameSize = 0;
	}
}

void CPMBRingBuffer::ReleaseCompletedFrames()
{
	const size_t MIN_FRAMES_TO_KEEP = 3;

	while (m_CompletedFrames.size() > MIN_FRAMES_TO_KEEP)
	{
		const auto& oldestFrame = m_CompletedFrames.front();

		GLenum result = glClientWaitSync(oldestFrame.fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
		if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED)
		{
			size_t frameEnd = oldestFrame.offset + oldestFrame.size;

			if (frameEnd > m_BufferSize) {
				frameEnd = frameEnd - m_BufferSize;
			}

			m_UsedSize -= oldestFrame.size;
			m_Tail = frameEnd;

			glDeleteSync(oldestFrame.fence);
			m_CompletedFrames.pop_front();
		}
		else
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
void CPMBRingBuffer::WaitForFrameIfOverlapping(size_t allocStart, size_t allocSize)
{
	// 遍历所有未完成的帧,检查是否与新分配区域重叠
	auto it = m_CompletedFrames.begin();
	while (it != m_CompletedFrames.end())
	{
		const auto& frame = *it;

		// 检测新分配区域 [allocStart, allocStart+allocSize) 是否与该帧 [frame.offset, frame.offset+frame.size) 重叠
		if (DoRangesOverlap(allocStart, allocSize, frame.offset, frame.size))
		{
			// 检测到重叠!必须等待该帧完成
			GLenum result = glClientWaitSync(frame.fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);

			if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED)
			{
				// 该帧已完成,释放它
				size_t frameEnd = frame.offset + frame.size;
				if (frameEnd > m_BufferSize) {
					frameEnd = frameEnd - m_BufferSize;
				}

				m_UsedSize -= frame.size;

				// 只有当该帧是最老的帧时才更新 m_Tail
				if (it == m_CompletedFrames.begin())
				{
					m_Tail = frameEnd;
				}

				glDeleteSync(frame.fence);
				it = m_CompletedFrames.erase(it);

				// 继续检查下一帧 (可能有多个重叠)
				continue;
			}
			else if (result == GL_TIMEOUT_EXPIRED || result == GL_WAIT_FAILED)
			{
				// 等待失败 - 这不应该发生,因为我们用了 GL_TIMEOUT_IGNORED
				gEngfuncs.Con_Printf("[%s] Warning: glClientWaitSync failed for overlapping frame (result=%d)\n",
					m_BufferName.c_str(), result);
			}
		}

		++it;
	}
}

IPMBRingBuffer* GL_CreatePMBRingBuffer(const char* name, size_t bufferSize, GLenum bufferTarget)
{
	return new CPMBRingBuffer(name, bufferSize, bufferTarget);
}