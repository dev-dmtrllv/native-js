#include "framework.hpp"
#include "AsyncWorker.hpp"
#include "js/Env.hpp"
#include "Event.hpp"
#include "constants.hpp"
#include "EventQueue.hpp"
#include "App.hpp"
#include "Worker.hpp"

namespace NativeJS
{
	AsyncWorker::AsyncWorker(App& app) :
		app_(app),
		mutex_(),
		cv_(),
		thread_([&]() { returnCode_ = entry(); }),
		returnCode_(0),
		isRunning_(false)
	{
		std::unique_lock lk(mutex_);
		cv_.wait(lk, [&]() { return isRunning_.load(std::memory_order::acquire); });
	}

	AsyncWorker::~AsyncWorker()
	{
		if (thread_.joinable())
			thread_.join();

		app_.logger().info("AsyncWorker exited with code ", returnCode_);
	}

	void AsyncWorker::terminate()
	{
		isRunning_.store(false, std::memory_order::release);
	}

	int AsyncWorker::entry()
	{
		isRunning_.store(true, std::memory_order::release);
		cv_.notify_all();

		Event* event = nullptr;

		while (app_.getAsyncWork(event))
		{
			if (!isRunning_.load(std::memory_order::acquire))
				break;

			if (event->type() == Event::Type::Async)
			{
				AsyncEvent& e = static_cast<AsyncEvent&>(*event);
				e.work_(std::addressof(e));
				e.worker_.postEvent(event);
			}
			else if(event->type() == Event::Type::Blocking)
			{
				BlockingEvent& e = static_cast<BlockingEvent&>(*event);
				e.work_(std::addressof(e));
				e.done_.store(true, std::memory_order::release);
				e.worker().blockingWorkCv_.notify_all();
			}
		}

		return 0;
	}
}