#include "framework.hpp"
#include "js/JSWindow.hpp"
#include "js/Env.hpp"
#include "App.hpp"
#include "js/JSConsole.hpp"

namespace NativeJS::JS
{
	Window::Window(const Env& env) : ObjectWrapper(env) { }

	Window::~Window() { }

	void Window::initializeProps()
	{
		loadMethod(onLoad_, "onLoad");
		loadMethod(onClose_, "onClose");
		loadMethod(onClosed_, "onClosed");
		loadMethod(render_, "render");
	}

	JS_METHOD_IMPL(Window::onLoad);
	JS_METHOD_IMPL(Window::onClose);
	JS_METHOD_IMPL(Window::onClosed);
	JS_METHOD_IMPL(Window::render);

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

	static void parseJsx(const Env& env, v8::Local<v8::Object> jsx)
	{
		v8::Local<v8::Value> type = getFromObject(env, jsx, "type").ToLocalChecked();
		v8::Local<v8::String> props = v8::JSON::Stringify(env.context(), getFromObject(env, jsx, "props").ToLocalChecked()).ToLocalChecked();
		puts("type");
		JS::Console::logValue(env, type);
		puts("props");
		JS::Console::logValue(env, props);
		v8::Local<v8::Value> children;
		if (getFromObject(env, jsx, "children", children))
		{
			if (children->IsArray())
			{
				v8::Local<v8::Array> arr = children.As<v8::Array>();
				const size_t l = arr->Length();
				for (size_t i = 0; i < l; i++)
				{
					v8::Local<v8::Value> child = arr->Get(env.context(), i).ToLocalChecked();
					puts("child");
					JS::Console::logValue(env, type);
				}
			}
		}

		if (type->IsFunction())
		{
			parseJsx(env, type.As<v8::Function>()->CallAsConstructor(env.context(), 0, nullptr).ToLocalChecked().As<v8::Object>());
		}
		else if(type->IsString())
		{
			puts("got string");
			auto s = parseString(env, type);
			puts(s.c_str());
		}
	};

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
					JS::Window* jsWin = win->registerJsObject(std::addressof(env.worker()), args.This());
					args.This()->SetInternalField(0, v8::External::New(env.isolate(), jsWin));
					args.This()->SetInternalField(1, v8::External::New(env.isolate(), win));
					jsWin->onLoad();
					v8::Local<v8::Object> jsx = jsWin->render().ToLocalChecked().As<v8::Object>();

					parseJsx(env, jsx);
				}
			}
		}
	}

	JS_CLASS_METHOD_IMPL(WindowClass::onShow)
	{
		NativeJS::Window* win = static_cast<NativeJS::Window*>(args.This()->GetInternalField(1).As<v8::External>()->Value());
		args.GetReturnValue().Set(env.doAsyncWork([](Event* event) { static_cast<AsyncEvent*>(event)->data<NativeJS::Window>()->show(); }, nullptr, win, true));
	}

	JS_CREATE_CLASS(WindowClass)
	{
		builder.setStaticMethod("create", onCreate);
		builder.setConstructor(ctor);
		builder.setMethod("onLoad");
		builder.setMethod("minimize");
		builder.setMethod("maximize");
		builder.setMethod("unmaximize"); // ? What to name ?
		builder.setMethod("show", onShow);
		builder.setMethod("close");
		builder.setMethod("onClose");
		builder.setMethod("onClosed");
		builder.setMethod("render");
		builder.setInternalFieldCount(2);
	}
}