#pragma once

#include "framework.hpp"
#include "js/JSUtils.hpp"

namespace NativeJS
{
	namespace JS
	{
		class Env;

		namespace NativeJSModule
		{
			v8::Local<v8::Module> create(const Env& env);
		}
	}
}