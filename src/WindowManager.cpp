#include "framework.hpp"
#include "App.hpp"
#include "WindowManager.hpp"

namespace NativeJS
{
	WindowManager::WindowManager(App& app) :
		app_(app)
	{

	}

	WindowManager::~WindowManager()
	{

	}

	size_t WindowManager::getWindowCount() const { return windows_.size(); }

	App& WindowManager::app() const
	{
		return app_;
	}

	Window* WindowManager::create(const std::string& title)
	{
		const size_t index = windows_.alloc(*this, title);
		Window* w = windows_.at(index);
		w->setIndex(index);
		w->initialize();
		windowsMap_.emplace(w->handle(), w);
		return w;
	}

	Window* WindowManager::create(std::string&& title)
	{
		const size_t index = windows_.alloc(*this, std::forward<std::string>(title));
		Window* w = windows_.at(index);
		w->setIndex(index);
		w->initialize();
		windowsMap_.emplace(w->handle(), w);
		return w;
	}

	void WindowManager::destroy(Window::Handle handle)
	{
		Window* window = getWindow(handle);
		if(window != nullptr)
		{
			windowsMap_.erase(handle);
			windows_.free(window->index());
		}
	}

	Window* WindowManager::getWindow(Window::Handle handle)
	{
		if(windowsMap_.contains(handle))
			return windowsMap_.at(handle);
		return nullptr;
	}
}