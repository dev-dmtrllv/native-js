#include "framework.hpp"
#include "EventQueue.hpp"

namespace NativeJS
{
	EventQueue::EventQueue(size_t capacity) :
		queue_(capacity),
		threadID_(std::this_thread::get_id())
	{

	}

	EventQueue::~EventQueue()
	{

	}

	bool EventQueue::postEvent(Event* event)
	{
		if (queue_.push(event))
		{
			cv_.notify_one();
			return true;
		}
		return false;
	}

	bool EventQueue::tryPopEvent(Event*& event)
	{
		if (size() > 0)
			return queue_.pop(event);
		return false;
	}

	bool EventQueue::popEvent(Event*& event)
	{
		if (queue_.pop(event))
			return true;

		std::unique_lock lk(mutex_);
		cv_.wait(lk, [&]() { return size() > 0; });
		return queue_.pop(event);
	} 
	
	bool EventQueue::popEvent(Event*& event, size_t tickTimeout)
	{
		using namespace std::chrono;

		if (queue_.pop(event))
			return true;

		std::unique_lock lk(mutex_);

		// ! wait_for is not accurate in the milliseconds
		if (cv_.wait_for(lk, std::chrono::milliseconds(tickTimeout), [&]() { return size() > 0; }))
			return queue_.pop(event);

		return false;
	}

	const size_t EventQueue::size() const
	{
		return queue_.size();
	}
}