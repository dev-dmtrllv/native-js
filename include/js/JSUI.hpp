#pragma once

namespace NativeJS
{
	namespace JS
	{
		class Env;
		class Object;

		namespace UI
		{
			void expose(const Env& env, Object& global);
		}
	}
}