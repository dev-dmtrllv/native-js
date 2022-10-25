#pragma once

#include "js/JSClass.hpp"

namespace NativeJS
{
	namespace JS
	{
		class Env;
		
		class Window : public ObjectWrapper
		{
		public:
			Window(const Env& env);
			Window(const Window&) = delete;
			Window(Window&&) = delete;
			virtual ~Window();

			virtual void initializeProps();

			JS_METHOD_DECL(onClose);
			JS_METHOD_DECL(onClosed);
			JS_METHOD_DECL(onLoad);
		};

		class WindowClass : public Class
		{
			JS_CLASS_BODY(WindowClass);

		private:
			JS_CLASS_METHOD(onCreate);
			JS_CLASS_METHOD(ctor);
			JS_CLASS_METHOD(onShow);
		};
	}
}