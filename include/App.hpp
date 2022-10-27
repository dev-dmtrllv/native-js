#pragma once

#include "framework.hpp"
#include "Worker.hpp"
#include "PersistentList.hpp"
#include "EventQueue.hpp"
#include "EventAllocator.hpp"
#include "Logger.hpp"
#include "AppConfig.hpp"
#include "lockfree/Queue.hpp"
#include "WindowManager.hpp"

namespace NativeJS
{
	class AsyncWorker;

	class App
	{
	private:
		static App* currentInstance_;

#ifdef _WINDOWS
		static LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static void CALLBACK timerProc(HWND, UINT, UINT_PTR, DWORD);
#endif

	public:
#ifdef _WINDOWS
		static const std::wstring WIN_CLASS_NAME;
#endif
		static App& createFromArgs(int argc, char** argv);
		static App& getInstance();
		static Logger& getAppLogger();
		static int terminate();

	private:
		App(int argc, char** argv, std::filesystem::path&& rootDir, const size_t maxAsyncWorkers, const size_t maxV8PlatformThreads, const size_t tickTimeout);
		App(const App&) = delete;
		App(App&&) = delete;

	public:
		~App();

		void run();

		Worker* createWorker(const std::filesystem::path& entry, Worker* parentWorker = nullptr);
		Worker* createWorker(std::filesystem::path&& entry, Worker* parentWorker = nullptr);
		bool destroyWorker(Worker* worker);

		std::vector<const char*> getAppArgs() const;
		size_t getTickTimeout() const;
		Logger& logger();
		const std::filesystem::path& rootDir() const;
		const AppConfig& appConfig() const;
		WindowManager& windowManager();

		bool getAsyncWork(Event*& event);
		bool postEvent(Event* event, bool onMainThread = false);

	private:
		void emitEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifdef _WINDOWS
		using ThreadID = DWORD;
#else
		using ThreadID = std::thread::id;
#endif
		const int argc_;
		char** argv_;
		const size_t tickTimeout_;
		ThreadID mainThreadID_;
		std::filesystem::path rootDir_;
		Logger& logger_;
		int exitCode_;
		std::vector<AsyncWorker*> asyncWorkers_;
		LockFree::Queue<Event*> asyncEventQueue_;
		std::mutex asyncMutex_;
		std::condition_variable asyncCV_;

		PersistentList<Worker> workers_;
		Worker* mainWorker_;

		EventQueue eventQueue_;
		EventAllocator events_;

		std::unique_ptr<v8::Platform> v8Platform_;

		AppConfig appConfig_;
		WindowManager windowManager_;

		std::unordered_map<UINT_PTR, TimeoutEvent*> timeoutEvents_;
		std::unordered_map<size_t, UINT_PTR> timeoutIDsToPtrs_;

		bool isTerminating_;

#ifdef _WINDOWS
		WNDCLASS wc_;
#endif
	};
}