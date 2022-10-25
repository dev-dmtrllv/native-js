#include "framework.hpp"
#include "js/JSWindow.hpp"
#include "js/Env.hpp"
#include "App.hpp"

namespace NativeJS::JS
{
	Window::Window(const Env& env) : ObjectWrapper(env) { }

	Window::~Window() { }

	void Window::initializeProps()
	{
		loadMethod(onLoad_, "onLoad");
		loadMethod(onClose_, "onClose");
		loadMethod(onClosed_, "onClosed");
	}

	JS_METHOD_IMPL(Window::onLoad);
	JS_METHOD_IMPL(Window::onClose);
	JS_METHOD_IMPL(Window::onClosed);

	JS_CLASS_METHOD_IMPL(WindowClass::onCreate)
	{
		const size_t l = args.Length();

		if (l < 2)
		{
			env.throwException("Not enough arguments!");
		}
		else
		{
			if (!args[0]->IsFunction())
			{
				env.throwException("First argument is not a class!");
			}
			else if (!args[1]->IsString())
			{
				env.throwException("Second argument is not a string!");
			}
			else
			{
				struct AsyncInfo
				{
					NativeJS::Window* win = nullptr;
					std::string title;
					v8::Persistent<v8::Function> jsClass;
				};

				// TODO: Replace the new AsyncInfo with a custom allocated one
				AsyncInfo* info = new AsyncInfo();
				info->title = parseString(env, args[1]);
				info->jsClass.Reset(env.isolate(), args[0].As<v8::Function>());

				v8::Local<v8::Promise> promise = env.doAsyncWork([](Event* event)
				{
					AsyncEvent* e = static_cast<AsyncEvent*>(event);
					AsyncInfo* info = e->data<AsyncInfo>();
					const Env& env = e->worker().env();
					info->win = env.app().windowManager().create(info->title);

				}, [](const WorkEvent& event)
				{
					const Env& env = event.worker().env();
					AsyncInfo* info = event.data<AsyncInfo>();
					std::vector<v8::Local<v8::Value>> winArgs = { v8::External::New(env.isolate(), info->win) };
					v8::Local<v8::Value> jsWin = info->jsClass.Get(env.isolate())->CallAsConstructor(env.context(), 1, winArgs.data()).ToLocalChecked();
					info->win->registerJsObject(std::addressof(event.worker()), jsWin);
					event.resolvePromise(jsWin);
					delete info;
				}, info, true);

				args.GetReturnValue().Set(promise);
			}
		}
	}

	JS_CLASS_METHOD_IMPL(WindowClass::ctor)
	{
		NativeJS::Window* win = nullptr;

		const size_t l = args.Length();

		if (l > 0)
		{
			if (args[0]->IsExternal())
			{
				args.This()->SetInternalField(0, args[0]);
			}
			else
			{
				env.doBlockingWork([](Event* event)
				{
					BlockingEvent* e = static_cast<BlockingEvent*>(event);
					NativeJS::Window** winPtr = e->data<NativeJS::Window*>();
					*winPtr = e->worker().app().windowManager().create("test");
				}, &win, true);

				if (win != nullptr)
				{
					args.This()->SetInternalField(0, v8::External::New(env.isolate(), win));
					win->registerJsObject(std::addressof(env.worker()), args.This());
				}
			}
		}
	}

	JS_CLASS_METHOD_IMPL(WindowClass::onShow)
	{
		NativeJS::Window* win = static_cast<NativeJS::Window*>(args.This()->GetInternalField(0).As<v8::External>()->Value());
		args.GetReturnValue().Set(env.doAsyncWork([](Event* event) { static_cast<AsyncEvent*>(event)->data<NativeJS::Window>()->show(); }, nullptr, win, true));
	}

	JS_CREATE_CLASS(WindowClass)
	{
		builder.setStaticMethod("create", onCreate);
		builder.setConstructor(ctor);
		builder.setMethod("onLoad");
		builder.setMethod("minimize");
		builder.setMethod("maximize");
		builder.setMethod("unmaximize"); // ??
		builder.setMethod("show", onShow);
		builder.setMethod("close");
		builder.setMethod("onClose");
		builder.setMethod("onClosed");
		builder.setInternalFieldCount(1);
	}
}