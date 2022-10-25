#pragma once

namespace NativeJS
{
	namespace JS
	{
		class Env;
		class Object;

		namespace Process
		{
			void expose(const Env& env, Object& global);
		}
	}
}