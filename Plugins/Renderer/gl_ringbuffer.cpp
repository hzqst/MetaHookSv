#include "gl_local.h"

// 环形分配器实现
bool CPMBRingBuffer::Initialize(size_t bufferSize)
{
	//Already initialized
	if (m_MappedPtr)
		return true;

	m_BufferSize = bufferSize;
	m_WriteOffset = 0;
	m_CurrentFrame = 0;

	// 初始化帧信息
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		m_Frames[i].startOffset = 0;
		m_Frames[i].endOffset = 0;
		m_Frames[i].fence = nullptr;
		m_Frames[i].active = false;
	}

	m_RingVBO = GL_GenBuffer();

	glBindBuffer(GL_ARRAY_BUFFER, m_RingVBO);

	// 创建persistent mapped buffer
	glBufferStorage(GL_ARRAY_BUFFER, m_BufferSize, nullptr,
		GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

	m_MappedPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, m_BufferSize,
		GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return m_MappedPtr != nullptr;
}

void CPMBRingBuffer::Shutdown()
{
	// 清理所有fence
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (m_Frames[i].fence)
		{
			glDeleteSync(m_Frames[i].fence);
			m_Frames[i].fence = nullptr;
		}
	}

	if (m_MappedPtr)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_RingVBO);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		m_MappedPtr = nullptr;
	}

	if (m_RingVBO)
	{
		GL_DeleteBuffer(m_RingVBO);
		m_RingVBO = 0;
	}
}

bool CPMBRingBuffer::Allocate(size_t size, size_t alignment, CPMBRingBuffer::Allocation &allocation)
{
	// 清理已完成的帧
	CleanupCompletedFrames();

	// 对齐到指定边界
	size_t alignedOffset = (m_WriteOffset + alignment - 1) & ~(alignment - 1);
	size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);

	// 检查是否需要wrap around
	if (alignedOffset + alignedSize > m_BufferSize)
	{
		// 尝试从头开始分配
		if (CanAllocateAt(0, alignedSize))
		{
			// 如果当前帧正在进行中，需要结束当前段
			if (m_Frames[m_CurrentFrame].active && m_Frames[m_CurrentFrame].endOffset == 0)
			{
				m_Frames[m_CurrentFrame].endOffset = m_WriteOffset;
			}

			m_WriteOffset = 0;
			alignedOffset = 0;
		}
		else
		{
			// 缓冲区满了，返回无效分配
			return false;
		}
	}

	// 检查是否与正在使用的区域重叠
	if (!CanAllocateAt(alignedOffset, alignedSize))
	{
		return false;
	}

	allocation.ptr = (char*)m_MappedPtr + alignedOffset;
	allocation.offset = alignedOffset;
	allocation.size = alignedSize;
	allocation.valid = true;

	m_WriteOffset = alignedOffset + alignedSize;
	return true;
}

void CPMBRingBuffer::BeginFrame()
{
	// 清理之前帧的fence
	auto& frame = m_Frames[m_CurrentFrame];
	if (frame.fence)
	{
		glDeleteSync(frame.fence);
		frame.fence = nullptr;
	}

	frame.startOffset = m_WriteOffset;
	frame.active = true;
}

void CPMBRingBuffer::EndFrame()
{
	auto& frame = m_Frames[m_CurrentFrame];
	frame.endOffset = m_WriteOffset;
	frame.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void CPMBRingBuffer::Reset()
{
	// 等待所有fence完成
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (m_Frames[i].fence)
		{
			glClientWaitSync(m_Frames[i].fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
			glDeleteSync(m_Frames[i].fence);
			m_Frames[i].fence = nullptr;
		}
		m_Frames[i].active = false;
	}

	m_WriteOffset = 0;
	m_CurrentFrame = 0;
}

bool CPMBRingBuffer::CanAllocateAt(size_t offset, size_t size)
{
	// 检查与正在使用的区域是否重叠
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		auto& frame = m_Frames[i];
		if (!frame.active) continue;

		size_t frameStart = frame.startOffset;
		size_t frameEnd = frame.endOffset;

		// 特殊处理当前正在构建的帧
		if (i == m_CurrentFrame && frameEnd == 0)
		{
			// 当前帧还在构建中，使用当前写入位置作为结束位置
			frameEnd = m_WriteOffset;
		}

		// 如果帧区域为空，跳过检查
		if (frameStart == frameEnd) continue;

		// 检查区域重叠
		if (frameStart <= frameEnd)
		{
			// 正常情况：[frameStart, frameEnd)
			// 检查新分配区域是否与已用区域重叠
			if (offset < frameEnd && offset + size > frameStart)
				return false;
		}
		else
		{
			// Wrap around情况：[frameStart, bufferSize) + [0, frameEnd)
			// 已用区域分为两段，检查新分配是否与任一段重叠
			bool overlapWithEnd = (offset < frameEnd);  // 与 [0, frameEnd) 重叠
			bool overlapWithStart = (offset < frameStart && offset + size > frameStart);  // 与 [frameStart, bufferSize) 重叠

			if (overlapWithEnd || overlapWithStart)
				return false;
		}
	}
	return true;
}

void CPMBRingBuffer::CleanupCompletedFrames()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		auto& frame = m_Frames[i];
		if (!frame.active || !frame.fence) continue;

		// 检查fence状态
		GLenum result = glClientWaitSync(frame.fence, 0, 0);
		if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED)
		{
			glDeleteSync(frame.fence);
			frame.fence = nullptr;
			frame.active = false;
		}
	}
}
