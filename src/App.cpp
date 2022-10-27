#include "framework.hpp"
#include "App.hpp"
#include "constants.hpp"
#include "js/Env.hpp"
#include "js/JSUtils.hpp"
#include "AsyncWorker.hpp"

namespace NativeJS
{
	static const std::string MAX_V8_WORKERS_STR = "MAX_V8_WORKERS_THREADS";
	static const std::string MAX_ASYNC_WORKERS_STR = "MAX_ASYNC_WORKERS";
	static const std::string TICK_TIMEOUT_STR = "TICK_TIMEOUT";

	App* App::currentInstance_ = nullptr;

#ifdef _WINDOWS
	const std::wstring App::WIN_CLASS_NAME = L"NATIVEJS";
#endif

	App& App::createFromArgs(int argc, char** argv)
	{
		assert(currentInstance_ == nullptr);
		size_t cores = std::thread::hardware_concurrency();
		const size_t half = cores / 2;
		size_t maxAsyncWorkers = half;
		size_t maxPlatformWorkers = half == 1 ? 1 : half - 1;
		size_t tickTimeout = 0;

		std::filesystem::path startDir;

		if (argc > 0 && argv[0][0] != '-')
		{
			startDir = std::filesystem::path(argv[0]).lexically_normal();
			if (startDir.is_relative())
				startDir = std::filesystem::current_path() / startDir;
			startDir = startDir.lexically_normal();
			if (!std::filesystem::exists(startDir))
				throw std::runtime_error("Path does not exists!");
		}
		else
		{
			startDir = std::filesystem::current_path();
		}

		char buf[16] = {};
		memset(buf, 0, 16);

		for (size_t i = 0; i < argc; i++)
		{
			std::string str = argv[i];
			if (str.starts_with(MAX_V8_WORKERS_STR))
			{
				std::string val(&str.data()[MAX_V8_WORKERS_STR.length() + 1]);
				std::stringstream sstream(val);
				sstream >> maxPlatformWorkers;
			}
			else if (str.starts_with(MAX_ASYNC_WORKERS_STR))
			{
				std::string val(&str.data()[MAX_ASYNC_WORKERS_STR.length() + 1]);
				std::stringstream sstream(val);
				sstream >> maxAsyncWorkers;
			}
			else if (str.starts_with(TICK_TIMEOUT_STR))
			{
				std::string val(&str.data()[TICK_TIMEOUT_STR.length() + 1]);
				std::stringstream sstream(val);
				sstream >> tickTimeout;
			}
		}

		App::currentInstance_ = new App(argc, argv, std::move(startDir), maxAsyncWorkers, maxPlatformWorkers, tickTimeout);
		return *App::currentInstance_;
	}

	App& App::getInstance()
	{
		assert(currentInstance_ != nullptr);
		return *App::currentInstance_;
	}

	Logger& App::getAppLogger()
	{
		assert(currentInstance_ != nullptr);
		return currentInstance_->logger_;
	}

	int App::terminate()
	{
		assert(currentInstance_ != nullptr);
		int exitCode = currentInstance_->exitCode_;
		delete currentInstance_;
		currentInstance_ = nullptr;
		return exitCode;
	}

	App::App(int argc, char** argv, std::filesystem::path&& rootDir, const size_t maxAsyncWorkers, const size_t maxV8PlatformThreads, const size_t tickTimeout) :
		argc_(argc),
		argv_(argv),
		tickTimeout_(tickTimeout),
		mainThreadID_(GetCurrentThreadId()),
		rootDir_(rootDir),
		logger_(Logger::get()),
		exitCode_(0),
		asyncWorkers_(),
		asyncEventQueue_(MAX_QUEUE_SIZE),
		eventQueue_(MAX_QUEUE_SIZE),
		v8Platform_(v8::platform::NewDefaultPlatform()),
		appConfig_(),
		windowManager_(*this),
		isTerminating_(false)
	{
#ifdef _WINDOWS
		logger().info(argc);

		logger().debug("Registering Windows Class...");
		memset(&wc_, 0, sizeof(WNDCLASS));
		wc_.lpfnWndProc = App::windowProc;
		wc_.hInstance = GetModuleHandle(NULL);
		wc_.lpszClassName = WIN_CLASS_NAME.c_str();

		RegisterClass(&wc_);
#endif

		logger().debug("Initializing Async Workers...");

		for (size_t i = 0; i < 4; i++)
			asyncWorkers_.emplace_back(new AsyncWorker(*this));

		logger().debug("Initializing v8 Platform...");
		v8::V8::InitializePlatform(v8Platform_.get());
		logger().debug("Initializing v8...");
		v8::V8::Initialize();

		JS::BaseEnv startupEnv_ = JS::BaseEnv(*this);

		JS::Env::Scope scope(startupEnv_);

		std::filesystem::path appJsonPath = rootDir_ / "app.json";
		std::ifstream is(appJsonPath);
		std::string jsonString((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
		v8::Local<v8::Value> config;
		if (!v8::JSON::Parse(startupEnv_.context(), JS::string(startupEnv_, jsonString)).ToLocal(&config))
			throw std::runtime_error("Could not parse app.json!");
		appConfig_.load(startupEnv_, config.As<v8::Object>());
	}

	App::~App()
	{
		isTerminating_ = true;

		if (mainWorker_ != nullptr)
			workers_.free(mainWorker_);

		workers_.forEach([&](Worker* worker)
		{
			if (!worker->isTerminated() && worker->isDetached())
			{
				int exitCode = 0;
				if (worker->terminate(exitCode))
				{
					logger().error("Worker exited with code ", exitCode);
				}
				else
				{
					logger().error("Could not terminate worker!");
				}
			}
		});

		logger().debug("Disposing v8...");
		v8::V8::Dispose();

		logger().debug("Disposing v8 Platform...");
		v8::V8::DisposePlatform();

		for (AsyncWorker* worker : asyncWorkers_)
			worker->terminate();

		asyncCV_.notify_all();

		for (AsyncWorker* worker : asyncWorkers_)
			delete worker;

#ifdef _WINDOWS
		logger().debug("Unregistering Windows Class...");
		UnregisterClass(WIN_CLASS_NAME.c_str(), GetModuleHandle(NULL));
#endif

		Logger::terminate();
	}

	Worker* App::createWorker(std::filesystem::path&& entry, Worker* parentWorker)
	{
		std::filesystem::path p;

		if (!entry.is_absolute())
			p = rootDir_ / entry;
		else
			p = entry;

		p = p.lexically_normal();

		size_t index = workers_.alloc(*this, std::move(p), parentWorker);
		Worker* worker = workers_.at(index);
		worker->index_ = index;
		return worker;
	}

	Worker* App::createWorker(const std::filesystem::path& entry, Worker* parentWorker)
	{
		std::filesystem::path p;

		if (!entry.is_absolute())
			p = rootDir_ / entry;
		else
			p = entry;

		p = p.lexically_normal();

		size_t index = workers_.alloc(*this, p, parentWorker);
		Worker* worker = workers_.at(index);
		worker->index_ = index;
		return worker;
	}

	bool App::destroyWorker(Worker* worker)
	{
		assert(worker != nullptr);
		int exitCode = 0;
		if (worker->terminate(exitCode))
		{
			logger_.info("Worker exited with code ", exitCode);
			workers_.free(worker->index_);
			return true;
		}
		return false;
	}

	void App::emitEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NativeEvent* event = events_.create<NativeEvent>(OSEvent { hwnd, uMsg, wParam, lParam }, workers_.size());
		workers_.forEach([&](Worker* worker)
		{
			if (!worker->postEvent(event))
				logger_.error("Could not post event!");
		});
	}

	bool App::postEvent(Event* event, bool onMainThread)
	{
		if (onMainThread)
		{
			if (eventQueue_.queue_.push(event))
			{
#ifdef _WINDOWS
				PostThreadMessage(mainThreadID_, WM_USER, 0, 0);
#endif
				return true;
			}
			return false;
		}
		else if (asyncEventQueue_.push(event))
		{
			asyncCV_.notify_one();
			return true;
		}
		return false;
	}

	bool App::getAsyncWork(Event*& event)
	{
		if (isTerminating_)
		{
			event = Event::getTerminateEvent();
			return true;
		}

		if (asyncEventQueue_.pop(event))
			return true;

		{
			std::unique_lock lk(asyncMutex_);
			asyncCV_.wait(lk, [&]() { return isTerminating_ || asyncEventQueue_.size() > 0; });
		}

		return asyncEventQueue_.pop(event);
	}

	WindowManager& App::windowManager()
	{
		return windowManager_;
	}

	size_t App::getTickTimeout() const
	{
		return tickTimeout_;
	}

	std::vector<const char*> App::getAppArgs() const
	{
		std::vector<const char*> vec{};
		for (size_t i = 0; i < argc_; i++)
			vec.emplace_back(argv_[i]);
		return vec;
	}

	Logger& App::logger()
	{
		return logger_;
	}

	const std::filesystem::path& App::rootDir() const
	{
		return rootDir_;
	}

	const AppConfig& App::appConfig() const
	{
		return appConfig_;
	}


#ifdef _WINDOWS
	void App::run()
	{
		mainWorker_ = createWorker(appConfig_.entry.file);

		MSG msg = { };
		bool isRunning = true;

		Event* event;

		const size_t wait = tickTimeout_ == 0 ? INFINITE : tickTimeout_;

		while (isRunning)
		{
			if (eventQueue_.size() == 0 && PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == 0)
				MsgWaitForMultipleObjects(0, NULL, FALSE, INFINITE, QS_ALLINPUT);

			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
			{
				logger().debug("got event ", msg.message);
				if (msg.message == WM_QUIT)
				{
					emitEvent(msg.hwnd, msg.message, msg.wParam, msg.lParam); // ???
				}
				else
				{
					BOOL didTranslate = TranslateMessage(&msg);
					LRESULT result = DispatchMessage(&msg);
				}
			}

			if (eventQueue_.tryPopEvent(event))
			{
				if (event->status() != Event::Status::Canceled)
				{
					switch (event->type())
					{
						case Event::Type::Native:
						{
							NativeEvent* e = static_cast<NativeEvent*>(event);
							OSEvent osEvent = e->event();
							switch (e->event().uMsg)
							{
								case WM_QUIT:
									isRunning = false;
									break;
								case WM_DESTROY:
									windowManager_.destroy(e->event().hwnd);
									if (windowManager_.getWindowCount() == 0)
									{
										PostQuitMessage(0);
									}
									break;
							}
							DefWindowProc(osEvent.hwnd, osEvent.uMsg, osEvent.wParam, osEvent.lParam);
							events_.remove(event);
						}
						break;
						case Event::Type::Async:
						{
							AsyncEvent* e = static_cast<AsyncEvent*>(event);
							e->work_(e);
							e->worker_.postEvent(e);
						}
						break;
						case Event::Type::Blocking:
						{
							BlockingEvent* e = static_cast<BlockingEvent*>(event);
							e->work_(e);
							e->done_.store(true, std::memory_order::release);
							e->worker_.blockingWorkCv_.notify_all();
						}
						break;
						case Event::Type::Timeout:
						{
							TimeoutEvent* e = static_cast<TimeoutEvent*>(event);
							if (e->type == TimeoutEvent::Type::CANCEL)
							{
								if (!timeoutIDsToPtrs_.contains(e->timeoutIndex))
								{
									printf("Could not get timer with id %zu\n", e->timeoutIndex);
								}
								else
								{
									UINT_PTR timerID = timeoutIDsToPtrs_.at(e->timeoutIndex);
									KillTimer(NULL, timerID);
									timeoutIDsToPtrs_.erase(e->timeoutIndex);
									timeoutEvents_.erase(timerID);
								}
							}
							else if (e->type == TimeoutEvent::Type::RESET)
							{

							}
							else
							{
								const size_t r = e->resolveTime.count();
								const size_t now = Utils::now<std::chrono::milliseconds>().count();
								if (now <= r)
								{
									static UINT_PTR timerCounter = 0;
									UINT_PTR timerID = SetTimer(nullptr, ++timerCounter, r - now, App::timerProc);
									if (timerID == 0)
									{
										puts("Could not create timer!");
									}
									else
									{
										puts("got timeout event");

										timeoutEvents_.emplace(timerID, e);
										timeoutIDsToPtrs_.emplace(e->timeoutIndex, timerID);
									}
								}
								else
								{
									puts("resolve directly :D");
								}
							}

						}
						break;
					}
				}
				else
				{
					logger_.debug("Event canceled!");
				}
			}
		}
	}

	void CALLBACK App::timerProc(HWND hwnd, UINT uMsg, UINT_PTR intPtr, DWORD index)
	{
		assert(currentInstance_);

		NativeJS::App& app = *currentInstance_;

		if (app.timeoutEvents_.contains(intPtr))
		{
			TimeoutEvent* e = app.timeoutEvents_.at(intPtr);

			e->env.worker().postEvent(e);

			if (!e->loop)
			{
				KillTimer(hwnd, intPtr);
			}
			else
			{
				SetTimer(hwnd, intPtr, e->duration, App::timerProc);
			}
		}
	}

	LRESULT CALLBACK App::windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		assert(currentInstance_);

		NativeJS::App& app = *currentInstance_;

		switch (uMsg)
		{
			case WM_CLOSE:
			case WM_MOUSEMOVE:
			case WM_DESTROY:
			{
				app.emitEvent(hwnd, uMsg, wParam, lParam);
				return 0;
			}

			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);
				FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 2));
				EndPaint(hwnd, &ps);
			}
			return 0;
		}

		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}
#endif
