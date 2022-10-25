#pragma once

#include "framework.hpp"
#include "js/JSUtils.hpp"
#include "js/Env.hpp"
#include "js/JSObject.hpp"
#include "App.hpp"
#include "Logger.hpp"
#include "js/JSApp.hpp"
#include "js/JSClass.hpp"
#include "js/JSWindow.hpp"

namespace NativeJS::JS::NativeJSModule
{

	v8::Local<v8::Module> create(const Env& env)
	{
		using namespace v8;

		std::vector<Local<String>> exports = {
			JS::string(env, "default"),
			JS::string(env, "App"),
			JS::string(env, "Window"),
			// JS::string(env, "Worker"),
		};

		Local<Module> module = Module::CreateSyntheticModule(env.isolate(), JS::string(env, "native-js"), exports, [](Local<Context> context, Local<Module> module) -> MaybeLocal<Value>
		{
			const Env& env = Env::fromContext(context);

			auto isolate = context->GetIsolate();

			const JS::Object defaultExport(env);

			auto exportValue = [&](const char* name, Local<v8::Value> val)
			{
				module->SetSyntheticModuleExport(env.isolate(), JS::string(env, name), val);
				defaultExport.set(name, val);
			};

			const JS::EnvClasses& classes = env.getJsClasses();

			exportValue("App", classes.appClass.getClass());
			exportValue("Window", classes.windowClass.getClass());
			// exportValue("Worker", classes.workerClass.getClass());

			bool isNothing = module->SetSyntheticModuleExport(env.isolate(), JS::string(env, "default"), *defaultExport).IsNothing();

			if (isNothing)
			{
				env.app().logger().error("Could not create default export for module \"native-js\"!");
				return MaybeLocal<Value>(False(isolate));
			}

			return MaybeLocal<Value>(True(isolate));
		});

		Maybe<bool> result = module->InstantiateModule(env.context(), Env::importModule);

		if (result.IsNothing())
		{
			throw std::runtime_error("Can't instantiate module!");
		}
		else if (!result.FromJust())
		{
			throw std::runtime_error("Module instantiation failed!");
		}
		else
		{
			return module;
		}
	}
}