#pragma once

#include "framework.hpp"
#include "Event.hpp"
#include "EventAllocator.hpp"

namespace NativeJS
{
	class App;
	class EventQueue;
	class BlockingEvent;
	
	namespace JS
	{
		class Env;
	}

	class Worker
	{
	public:
		Worker(App& app, const std::filesystem::path& entry, Worker* parent);
		Worker(App& app, std::filesystem::path&& entry, Worker* parent);
		Worker(const Worker&) = delete;
		Worker(Worker&&) = delete;
		~Worker();

		bool terminate(int& exitCode);
		bool postEvent(Event* event);

		bool doAsyncWork(WorkCallback work, ResolverCallback resolver, void* data = nullptr, bool onMainThread = false);
		bool doBlockingWork(WorkCallback work, void* data = nullptr, bool onMainThread = false);

		inline const JS::Env& env() const { assert(env_); return *env_; }
		inline App& app() const { return app_; }

	protected:
		int entry();

	private:
		App& app_;
		std::filesystem::path entry_;
		Worker* parentWorker_;
		size_t index_;
		std::mutex mutex_;
		std::condition_variable cv_;
		std::thread thread_;
		size_t returnCode_;
		std::atomic<bool> isRunning_;
		std::atomic<bool> isBlocked_;
		std::mutex blockingWorkMutex_;
		std::condition_variable blockingWorkCv_;

		EventQueue* eventQueue_;
		EventAllocator events_;

		JS::Env* env_;

		friend class App;
		friend class JS::Env;
		friend class BlockingEvent;
		friend class AsyncWorker;
	};
}