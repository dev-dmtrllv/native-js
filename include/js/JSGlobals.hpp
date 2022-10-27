#pragma once

namespace NativeJS
{
	namespace JS
	{
		class Env;
		class Object;

		namespace JSGlobals
		{
			void expose(const Env& env, Object& obj);
		}
	}
}