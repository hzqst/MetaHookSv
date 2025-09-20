#include "gl_local.h"

// 环形分配器实现
bool CPMBRingBuffer::Initialize(size_t bufferSize)
{
	//Already initialized
	if (m_RingVBO)
		return true;

	m_BufferSize = bufferSize;
	m_Head = 0;
	m_Tail = 0;
	m_UsedSize = 0;
	m_CurrFrameSize = 0;

	// 清空已完成帧列表
	m_CompletedFrames.clear();

	m_RingVBO = GL_GenBuffer();

	glBindBuffer(GL_ARRAY_BUFFER, m_RingVBO);

	// 创建persistent mapped buffer
	glBufferStorage(GL_ARRAY_BUFFER, m_BufferSize, nullptr,
		GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return true;
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

	if (m_RingVBO)
	{
		GL_DeleteBuffer(m_RingVBO);
		m_RingVBO = 0;
	}

	m_Head = 0;
	m_Tail = 0;
	m_UsedSize = 0;
	m_CurrFrameSize = 0;
}

void CPMBRingBuffer::Commit(CPMBRingBuffer::Allocation& allocation)
{
	if (glUnmapNamedBuffer)
	{
		glUnmapNamedBuffer(m_RingVBO);
	}
	else
	{
		// We must unbind VAO because otherwise we will break the bindings
		GL_BindVAO(0);
		glBindBuffer(GL_ARRAY_BUFFER, m_RingVBO);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

bool CPMBRingBuffer::Allocate(size_t size, size_t alignment, CPMBRingBuffer::Allocation& allocation)
{
	// 释放已完成的帧
	ReleaseCompletedFrames();

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
			// 有足够空间在当前位置分配
			size_t offset = alignedHead;
			size_t adjustedSize = size + (alignedHead - m_Head);

			if (glMapNamedBufferRange)
			{
				allocation.ptr = glMapNamedBufferRange(m_RingVBO, offset, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				allocation.offset = offset;
				allocation.size = size;
				allocation.valid = true;
			}
			else
			{
				// We must unbind VAO because otherwise we will break the bindings
				GL_BindVAO(0);
				glBindBuffer(GL_ARRAY_BUFFER, m_RingVBO);
				glMapBufferRange(GL_ARRAY_BUFFER, offset, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}

			m_Head += adjustedSize;
			m_UsedSize += adjustedSize;
			m_CurrFrameSize += adjustedSize;

			return true;
		}
		else if (size <= m_Tail)
		{
			// 环绕分配：从缓冲区开头分配
			size_t addSize = (m_BufferSize - m_Head) + size;


			if (glMapNamedBufferRange)
			{
				allocation.ptr = glMapNamedBufferRange(m_RingVBO, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				allocation.offset = 0;
				allocation.size = size;
				allocation.valid = true;
			}
			else
			{
				// We must unbind VAO because otherwise we will break the bindings
				GL_BindVAO(0);
				glBindBuffer(GL_ARRAY_BUFFER, m_RingVBO);
				glMapBufferRange(GL_ARRAY_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}

			m_UsedSize += addSize;
			m_CurrFrameSize += addSize;
			m_Head = size;

			return true;
		}
	}
	else if (alignedHead + size <= m_Tail)
	{
		// 情况2: 环绕布局 [####Head----Tail####]
		size_t offset = alignedHead;
		size_t adjustedSize = size + (alignedHead - m_Head);

		if (glMapNamedBufferRange)
		{
			allocation.ptr = glMapNamedBufferRange(m_RingVBO, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			allocation.offset = offset;
			allocation.size = size;
			allocation.valid = true;
		}
		else
		{
			// We must unbind VAO because otherwise we will break the bindings
			GL_BindVAO(0);
			glBindBuffer(GL_ARRAY_BUFFER, m_RingVBO);
			glMapBufferRange(GL_ARRAY_BUFFER, offset, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		m_Head += adjustedSize;
		m_UsedSize += adjustedSize;
		m_CurrFrameSize += adjustedSize;

		return true;
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
}

void CPMBRingBuffer::EndFrame()
{
	// 只有非零大小的帧才需要创建fence
	if (m_CurrFrameSize > 0)
	{
		GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		if (fence)
		{
			m_CompletedFrames.emplace_back(fence, m_Head, m_CurrFrameSize);
		}
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
	// 释放所有已完成的帧
	while (!m_CompletedFrames.empty())
	{
		const auto& oldestFrame = m_CompletedFrames.front();

		// 检查fence状态
		GLenum result = glClientWaitSync(oldestFrame.fence, 0, 0);
		if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED)
		{
			// GPU已完成处理该帧，可以安全释放
			m_UsedSize -= oldestFrame.size;
			m_Tail = oldestFrame.offset;

			glDeleteSync(oldestFrame.fence);
			m_CompletedFrames.pop_front();
		}
		else
		{
			// GPU还在处理，停止检查
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
