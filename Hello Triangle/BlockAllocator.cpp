#include "BlockAllocator.h"

inline void vke::Chunk::deallocate()
{
	auto shared = m_block.lock();
	if (m_allocated && shared)
	{
		shared->deallocateChunk(offset);
	}
	else
	{
		m_allocated = false;
	}
}
