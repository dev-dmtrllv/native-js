#pragma once

#include "framework.hpp"
#include "js/JSWindow.hpp"

namespace NativeJS
{
	class App;
	class WindowManager;
	class Worker;

	namespace JS
	{
		class Env;
	};

	class Window
	{
	public:
#ifdef _WINDOWS
		using Handle = HWND;
#endif
		Window(WindowManager& windowManager, const std::string& title);
		Window(WindowManager& windowManager, std::string&& title);
		Window(const Window&) = delete;
		Window(Window&&) = delete;
		~Window();

		bool isInitialized() const;

		bool initialize();
		void show();
		void close();

		inline const size_t& index() const { return index_; }
		inline const Handle& handle() const { return handle_; }

		void registerJsObject(Worker* worker, v8::Local<v8::Value> jsWindow);
		JS::Window* getJsObject(Worker* worker);
		JS::Window* getJsObject(Worker& worker);

	private:
		template<typename Callback>
		inline void callThreadSafe(Callback callback)
		{
			{
				std::unique_lock lk(mutex_);
				cv_.wait(lk, [&]() { return !isLocked_.load(std::memory_order::acquire); });
				isLocked_.store(true, std::memory_order::release);
				callback();
			}
			isLocked_.store(false, std::memory_order::release);
			cv_.notify_one();
		}

		inline void setIndex(size_t index) const { index_ = index; };

		mutable size_t index_;
		WindowManager& windowManager_;
		std::string title_;
		Handle handle_;

		bool isInitialized_;

		std::unordered_map<Worker*, JS::Window> jsObjects_;
		std::mutex mutex_;
		std::atomic<bool> isLocked_;
		std::condition_variable cv_;

		friend class WindowManager;
	};
}