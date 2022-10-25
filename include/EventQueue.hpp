#pragma once

#include "framework.hpp"
#include "lockfree/Queue.hpp"
#include "Event.hpp"

namespace NativeJS
{
	class EventQueue
	{
	public:
		EventQueue(size_t capacity);
		EventQueue(const EventQueue&) = delete;
		EventQueue(EventQueue&&) = delete;
		~EventQueue();

		bool postEvent(Event* event);
		bool tryPopEvent(Event*& event);
		bool popEvent(Event*& event);
		bool popEvent(Event*& event, size_t tickTimeout);

		const size_t size() const;

	private:
		LockFree::Queue<Event*> queue_;
		std::mutex mutex_;
		std::condition_variable cv_;
		std::thread::id threadID_;

		friend class App;
	};
}