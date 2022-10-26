#include "framework.hpp"
#include "Worker.hpp"
#include "js/Env.hpp"
#include "Event.hpp"
#include "constants.hpp"
#include "EventQueue.hpp"
#include "App.hpp"

namespace NativeJS
{
	Worker::Worker(App& app, std::filesystem::path&& envEntry, Worker* parent) :
		app_(app),
		entry_(envEntry),
		parentWorker_(parent),
		index_(0),
		mutex_(),
		cv_(),
		thread_([&]() { returnCode_ = entry(); }),
		returnCode_(0),
		eventQueue_(nullptr),
		isRunning_(false),
		isBlocked_(false),
		blockingWorkMutex_(),
		blockingWorkCv_()
	{
		assert(entry_.is_absolute());
		std::unique_lock lk(mutex_);
		cv_.wait(lk, [&]() { return isRunning_.load(std::memory_order::acquire); });
	}

	Worker::Worker(App& app, const std::filesystem::path& envEntry, Worker* parent) :
		app_(app),
		entry_(envEntry),
		parentWorker_(parent),
		index_(0),
		mutex_(),
		cv_(),
		thread_([&]() { returnCode_ = entry(); }),
		returnCode_(0),
		eventQueue_(nullptr),
		isRunning_(false)
	{
		std::unique_lock lk(mutex_);
		cv_.wait(lk, [&]() { return isRunning_.load(std::memory_order::acquire); });
	}

	Worker::~Worker()
	{
		int returnCode = 0;
		if (terminate(returnCode))
			app_.logger().info("Worker exited with code ", returnCode);
	}

	bool Worker::terminate(int& exitCode)
	{
		if (thread_.joinable())
		{
			eventQueue_->postEvent(Event::getTerminateEvent());
			thread_.join();
			exitCode = returnCode_;
			return true;
		}
		return false;
	}

	int Worker::entry()
	{
		eventQueue_ = new EventQueue(MAX_QUEUE_SIZE);

		isRunning_.store(true, std::memory_order::release);
		cv_.notify_all();
		printf("%zu\n", this);
		JS::Env env = JS::Env(app_, this, entry_, parentWorker_);

		env_ = &env;

		JS::Env::Scope scope(env);

		env.loadEntryModule();

		Event* event;

		bool terminated = false;

		const size_t tickTimeout = app_.getTickTimeout();

		const auto pop = [&]()
		{
			if (tickTimeout != 0)
				return eventQueue_->popEvent(event, tickTimeout);
			return eventQueue_->popEvent(event);
		};

		while (!terminated)
		{
			JS::Env::Scope scope(env);
			if (pop())
			{
				switch (event->type())
				{
					case Event::Type::Terminate:
					{
						terminated = true;
						break;
					}
					case Event::Type::Native:
					{
						NativeEvent& e = event->as<NativeEvent>();

						const bool wasLastProcessed = e.process([&](const OSEvent& nativeEvent)
						{
							auto getWindow = [&](){ return env.app().windowManager().getWindow(nativeEvent.hwnd)->getJsObject(env.worker()); };

							switch (nativeEvent.uMsg)
							{
								case WM_DESTROY:
								{
									NativeJS::JS::Window* win = getWindow();

									if (win != nullptr)
										win->onClosed();
								}
								break;
								case WM_CLOSE:
								{
									NativeJS::JS::Window* win = getWindow();
									if (win != nullptr)
										win->onClose({ env.createEvent(event) });
								}
								break;
								case WM_QUIT:
								{
									if (env_->isJsAppInitialized())
										env.jsApp().onQuit({ env.createEvent(event) });
								}
								break;
							}
						});

						if (wasLastProcessed)
						{
							app_.postEvent(std::addressof(e), true);
						}
					}
					break;
					case Event::Type::Async:
					{
						AsyncEvent& e = event->as<AsyncEvent>();
						e.resolve();
						events_.remove(event);
					}
					break;
					case Event::Type::Message:
					{
						MessageEvent& e = event->as<MessageEvent>();
						if (std::addressof(e.sender()) != this)
						{
							env.emitMessage(e);
							e.sender().postEvent(event);
						}
						else
						{
							e.resolvePromise();
							events_.remove(event);
						}
					}
					break;
					case Event::Type::Timeout:
					{
						TimeoutEvent& e = event->as<TimeoutEvent>();
						env.resolveTimeout(e.timeoutIndex);
					}
					break;
				}

				if (terminated)
					break;
			}
			else
			{
				if (env_->isJsAppInitialized())
					env_->jsApp().onTick();
			}

			if (terminated)
				break;
		}

		isRunning_.store(false, std::memory_order::release);

		return 0;
	}

	bool Worker::postEvent(Event* event)
	{
		assert(eventQueue_);
		if (!eventQueue_->postEvent(event))
			return false;
		return true;
	}

	bool Worker::doAsyncWork(WorkCallback work, ResolverCallback resolver, void* data, bool onMainThread)
	{
		AsyncEvent* event = events_.create<AsyncEvent>(*this, work, resolver, data);
		if (!app_.postEvent(event, onMainThread))
		{
			events_.remove(event);
			return false;
		}
		return true;
	}

	bool Worker::doBlockingWork(WorkCallback work, void* data, bool onMainThread)
	{
		isBlocked_.store(true, std::memory_order::release);
		BlockingEvent* e = events_.create<BlockingEvent>(*this, work, data);
		if (app_.postEvent(e, onMainThread))
		{
			std::unique_lock lk(blockingWorkMutex_);
			blockingWorkCv_.wait(lk, [&]() { return e->isDone(); });
			isBlocked_.store(false, std::memory_order::release);
			events_.remove(e);
			return true;
		}
		isBlocked_.store(false, std::memory_order::release);
		events_.remove(e);
		return false;
	}
}