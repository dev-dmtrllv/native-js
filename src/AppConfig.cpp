#include "framework.hpp"
#include "AppConfig.hpp"
#include "js/JSUtils.hpp"
#include "js/BaseEnv.hpp"
#include "App.hpp"

namespace NativeJS
{
	void AppConfig::load(const JS::BaseEnv& env, v8::Local<v8::Object> obj)
	{
		assert(!isLoaded_);

		JS::parseString(env, JS::getFromObject(env, obj, "name").ToLocalChecked(), name);
		JS::parseString(env, JS::getFromObject(env, obj, "type").ToLocalChecked(), type);

		Logger& logger = env.app().logger();

		v8::Local<v8::Value> entryObj = JS::getFromObject(env, obj, "entry").ToLocalChecked();
		if (entryObj.IsEmpty())
		{
			logger.warn("No entry provided!");
		}
		else if (entryObj->IsString())
		{
			JS::parseString(env, entryObj, entry.file);
			entry.exportName = "default";
		}
		else
		{
			v8::Local<v8::Value> file = JS::getFromObject(env, entryObj.As<v8::Object>(), "file").ToLocalChecked();
			if (file.IsEmpty())
			{
				logger.warn("No entry file provided!");
			}
			else
			{
				JS::parseString(env, file, entry.file);
				v8::Local<v8::Value> exportsName = JS::getFromObject(env, entryObj.As<v8::Object>(), "export").ToLocalChecked();
				if (!exportsName.IsEmpty())
					JS::parseString(env, exportsName, entry.exportName);
				else
					entry.exportName = "default";
			}

		}

		v8::Local<v8::Value> resolvesObj = JS::getFromObject(env, obj, "resolve").ToLocalChecked().As<v8::Array>();
		if (!resolvesObj.IsEmpty())
		{
			v8::Local<v8::Array> resolvesArr = resolvesObj.As<v8::Array>();
			const uint32_t l = resolvesArr->Length();

			resolve.resize(l);

			for (uint32_t i = 0; i < l; i++)
				JS::parseString(env, resolvesArr->Get(env.context(), i).ToLocalChecked(), resolve[i]);
		}

		isLoaded_ = true;
	}
}