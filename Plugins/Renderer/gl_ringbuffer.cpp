#include "gl_local.h"

// 环形分配器实现
bool CPMBRingBuffer::Initialize(const char *name, size_t bufferSize, int bufferType)
{
	//Already initialized
	if (m_MappedPtr)
		return true;

	m_BufferName = name;
	m_BufferSize = bufferSize;
	m_Head = 0;
	m_Tail = 0;
	m_UsedSize = 0;
	m_CurrFrameSize = 0;
	m_FrameStartOffset = 0;

	// 清空已完成帧列表
	m_CompletedFrames.clear();

	if (bufferType == GL_ARRAY_BUFFER)
	{
		m_hVBO = GL_GenBuffer();
	}
	else if (bufferType == GL_ELEMENT_ARRAY_BUFFER)
	{
		m_hEBO = GL_GenBuffer();
	}

	GL_BindVAO(0);

	if (bufferType == GL_ARRAY_BUFFER)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_hVBO);

		glBufferStorage(GL_ARRAY_BUFFER, m_BufferSize, nullptr,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

		m_MappedPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, m_BufferSize,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (glObjectLabel)
		{
			glObjectLabel(GL_BUFFER, m_hEBO, -1, m_BufferName.c_str());
		}
	}
	else if (bufferType == GL_ELEMENT_ARRAY_BUFFER)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_hEBO);

		glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, m_BufferSize, nullptr,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

		m_MappedPtr = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, m_BufferSize,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		if (glObjectLabel)
		{
			glObjectLabel(GL_BUFFER, m_hEBO, -1, m_BufferName.c_str());
		}
	}

	return m_MappedPtr != nullptr;
}

void CPMBRingBuffer::Shutdown()
{
	// 清理所有fence
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

		if (m_hVBO)
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_hVBO);
			glUnmapBuffer(GL_ARRAY_BUFFER);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		else if(m_hEBO)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_hEBO);
			glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

		m_MappedPtr = nullptr;
	}

	if (m_hVBO)
	{
		GL_DeleteBuffer(m_hVBO);
		m_hVBO = 0;
	}
	else if (m_hEBO)
	{
		GL_DeleteBuffer(m_hEBO);
		m_hEBO = 0;
	}

	m_Head = 0;
	m_Tail = 0;
	m_UsedSize = 0;
	m_CurrFrameSize = 0;
}

bool CPMBRingBuffer::Allocate(size_t size, size_t alignment, CPMBRingBuffer::Allocation& allocation)
{
	// 释放已完成的帧
	//ReleaseCompletedFrames();

	// 验证参数
	if (size == 0)
		return false;
	if (!IsPowerOfTwo(alignment))
		return false;

	// 对齐size
	size = AlignUp(size, alignment);

	// 检查是否有足够空间
	if (m_UsedSize + size > m_BufferSize)
	{
		return false;
	}

	size_t alignedHead = AlignUp(m_Head, alignment);

	if (m_Head >= m_Tail)
	{
		// 情况1: 正常布局 [----Tail####Head----]
		if (alignedHead + size <= m_BufferSize)
		{
			// 在当前位置分配
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
			// 环绕到开头分配
			size_t wastedSpace = m_BufferSize - m_Head;

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
		// 情况2: 环绕布局 [####Head----Tail####]
		if (alignedHead + size <= m_Tail)
		{
			// 在 Head 和 Tail 之间分配
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
			// 新增：尝试在 Tail 之后分配
			size_t alignedTail = AlignUp(m_Tail, alignment);
			if (alignedTail + size <= m_BufferSize)
			{
				// 浪费掉 Head 到 Tail 之间的空间
				size_t wastedSpace = m_Tail - m_Head;

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

	// 没有足够空间
	return false;
}

void CPMBRingBuffer::BeginFrame()
{
	// 释放已完成的帧
	ReleaseCompletedFrames();

	// 重置当前帧大小
	m_CurrFrameSize = 0;
	m_FrameStartOffset = m_Head;
}

void CPMBRingBuffer::EndFrame()
{
	// 只有非零大小的帧才需要创建fence
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

void CPMBRingBuffer::Reset()
{
	// 等待所有fence完成
	for (auto& frame : m_CompletedFrames)
	{
		if (frame.fence)
		{
			glClientWaitSync(frame.fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
			glDeleteSync(frame.fence);
		}
	}
	m_CompletedFrames.clear();

	m_Head = 0;
	m_Tail = 0;
	m_UsedSize = 0;
	m_CurrFrameSize = 0;
}

void CPMBRingBuffer::ReleaseCompletedFrames()
{
	// 保留最近N帧不释放（N取决于GPU pipeline深度）
	const size_t MIN_FRAMES_TO_KEEP = 0;

	while (m_CompletedFrames.size() > MIN_FRAMES_TO_KEEP)
	{
		const auto& oldestFrame = m_CompletedFrames.front();

		GLenum result = glClientWaitSync(oldestFrame.fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
		if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED)
		{
			// 只有当fence完成且已经过了足够帧数才释放
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

	// 如果缓冲区为空，重置Head/Tail指针
	if (IsEmpty())
	{
		m_CompletedFrames.clear();
		m_Head = 0;
		m_Tail = 0;
	}
}
