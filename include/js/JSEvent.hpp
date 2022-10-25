#pragma once

#include "js/JSClass.hpp"

namespace NativeJS
{
	namespace JS
	{
		class EventClass : public Class
		{
			JS_CLASS_BODY(EventClass);

		private:
			JS_CLASS_METHOD(ctor);
			JS_CLASS_METHOD(cancel);
		};
	}
}