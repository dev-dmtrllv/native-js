#include "framework.hpp"
#include "js/JSApp.hpp"
#include "js/Env.hpp"
#include "Worker.hpp"
#include "Event.hpp"

namespace NativeJS
{
	namespace JS
	{
		App::App(const Env& env) : ObjectWrapper(env), isInitialized_(false) { }

		void App::initializeProps()
		{
			loadMethod(onLoad_, "onLoad");
			loadMethod(onTick_, "onTick");
			loadMethod(onQuit_, "onQuit");
			isInitialized_ = true;
		}

		bool App::isInitialized() const { return isInitialized_; }

		JS_METHOD_IMPL(App::onLoad);
		JS_METHOD_IMPL(App::onTick);
		JS_METHOD_IMPL(App::onQuit);

		JS_CLASS_METHOD_IMPL(AppClass::onInit)
		{
			v8::Local<v8::Array> arr = v8::Array::New(env.isolate(), 2);
			arr->Set(env.context(), 0, args[0]);
			arr->Set(env.context(), 1, args[1]);
			v8::Persistent<v8::Array>* p = new v8::Persistent<v8::Array>(env.isolate(), arr);
			v8::Local<v8::Promise> promise = env.doAsyncWork([](Event* e) { }, [](const WorkEvent& e)
			{
				const Env& env = e.worker().env();
				v8::Persistent<v8::Array>* ptr = e.data<v8::Persistent<v8::Array>>();
				v8::Local<v8::Array> args = ptr->Get(env.isolate());
				v8::Local<v8::Function> ctor = args->Get(env.context(), 0).ToLocalChecked().As<v8::Function>();
				v8::Local<v8::Value> appObj = ctor->CallAsConstructor(env.context(), 0, nullptr).ToLocalChecked();
				env.initializeJSApp(appObj);
				e.resolvePromise(appObj);
				env.jsApp().onLoad({ args->Get(env.context(), 1).ToLocalChecked() });
			}, p, true);
			args.GetReturnValue().Set(promise);
		}

		JS_CLASS_METHOD_IMPL(AppClass::onGet)
		{
			if (!env.isJsAppInitialized())
			{

			}
			else
			{
				args.GetReturnValue().Set(env.jsApp().value());
			}
		}

		JS_CREATE_CLASS(AppClass)
		{
			builder.setStaticMethod("initialize", onInit, this);
			builder.setStaticMethod("get", onGet, this);
			builder.setMethod("onLoad");
			builder.setMethod("onTick");
			builder.setMethod("onQuit");
		}
	}
}