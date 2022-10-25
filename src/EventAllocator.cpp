#include "framework.hpp"
#include "EventAllocator.hpp"

namespace NativeJS
{
	EventAllocator::EventAllocator()
	{

	}

	bool EventAllocator::remove(Event* e)
	{
		return list_.free(e->index_);
	}
}