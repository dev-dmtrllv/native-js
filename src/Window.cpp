#include "framework.hpp"
#include "Window.hpp"
#include "WindowManager.hpp"
#include "App.hpp"
#include "utils.hpp"
#include "JS/Env.hpp"

namespace NativeJS
{
	class App;

	Window::Window(WindowManager& windowManager, const std::string& title) :
		index_(0),
		title_(title),
		windowManager_(windowManager),
		handle_(0),
		isLocked_(false)
	{

	}

	Window::Window(WindowManager& windowManager, std::string&& title) :
		index_(0),
		title_(title),
		windowManager_(windowManager),
		handle_(0),
		isLocked_(false)
	{

	}

	Window::~Window()
	{
		
	}

	void Window::registerJsObject(Worker* worker, v8::Local<v8::Value> jsWindow)
	{
		callThreadSafe([&]()
		{
			jsObjects_.emplace(worker, worker->env());
			jsObjects_.at(worker).wrap(jsWindow);
		});
	}

	JS::Window* Window::getJsObject(Worker* worker)
	{
		if (jsObjects_.contains(worker))
			return std::addressof(jsObjects_.at(worker));
		return nullptr;
	}

	JS::Window* Window::getJsObject(Worker& worker)
	{
		return getJsObject(std::addressof(worker));
	}

	bool Window::initialize()
	{
		if (isInitialized_)
			return false;
#ifdef _WINDOWS
		std::wstring wTitle = Utils::toWString(title_.c_str());
		handle_ = CreateWindowEx(0, App::WIN_CLASS_NAME.c_str(), wTitle.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
#endif
	}

	void Window::show()
	{
		if (!handle_)
			windowManager_.app().logger().warn("Tried to show a window that is not initialized!");
#ifdef _WINDOWS
		ShowWindow(handle_, SW_SHOWDEFAULT);
#endif
	}

	void Window::close()
	{
		if (!handle_)
			windowManager_.app().logger().warn("Tried to close a window that is not initialized!");
#ifdef _WINDOWS
		CloseWindow(handle_);
#endif	
	}
}