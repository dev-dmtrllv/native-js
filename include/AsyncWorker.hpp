#pragma once

#include "framework.hpp"

namespace NativeJS
{
	class App;
	class Event;
	class EventQueue;

	class AsyncWorker
	{
	public:
		AsyncWorker(App& app);
		AsyncWorker(const AsyncWorker&) = delete;
		AsyncWorker(AsyncWorker&&) = delete;
		~AsyncWorker();

		void terminate();
		
	protected:
		int entry();

	private:
		App& app_;
		std::mutex mutex_;
		std::condition_variable cv_;
		std::thread thread_;
		size_t returnCode_;
		std::atomic<bool> isRunning_;
	};
}