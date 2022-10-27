#pragma once

#include "concepts.hpp"
#include "Event.hpp"
#include "PersistentList.hpp"

namespace NativeJS
{
	class EventAllocator
	{
	public:
		EventAllocator();
		EventAllocator(const EventAllocator&) = delete;
		EventAllocator(EventAllocator&&) = delete;

		inline size_t size() const { return list_.size(); }

		template<typename T, class... Args>
			requires Concepts::Extends<Event, T> && std::constructible_from<T, Args...>
		T* create(Args&&... args)
		{
			size_t index = list_.alloc(static_cast<Event*>(new T(std::forward<Args>(args)...)));
			T* event = static_cast<T*>(*(list_.at(index)));
			event->index_ = index;
			return event;
		}

		bool remove(Event* e);

		template<typename Callback>
		void forEach(Callback callback) const
		{
			list_.forEach([&](Event** event){ callback(*event); });
		}

	private:
		PersistentList<Event*> list_;
	};
}