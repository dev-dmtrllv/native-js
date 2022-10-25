#pragma once

#include "Window.hpp"
#include "PersistentList.hpp"

namespace NativeJS
{
	class App;

	class WindowManager
	{
	public:
		WindowManager(App& app);
		WindowManager(const WindowManager&) = delete;
		WindowManager(WindowManager&&) = delete;
		~WindowManager();

		size_t getWindowCount() const;

		Window* create(const std::string& string);
		Window* create(std::string&& string);

		void destroy(Window::Handle handle);

		App& app() const;
		Window* getWindow(Window::Handle handle);


	private:
		App& app_;
		PersistentList<Window> windows_;
		std::unordered_map<Window::Handle, Window*> windowsMap_;
	};
}