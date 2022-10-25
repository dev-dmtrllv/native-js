#pragma once

#include "framework.hpp"
#include "js/JSObject.hpp"

namespace NativeJS
{
	namespace JS
	{
		class Env;

		namespace Console
		{
			std::string parse(const Env& env, v8::Local<v8::Value> val, size_t indentCount = 0, bool isObjectVal = false, bool skipIndent = false);
			void logValue(const Env& env, v8::Local<v8::Value> val);

			void expose(const Env& env, JS::Object& global);
		}
	};
};