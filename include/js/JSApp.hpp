#pragma once

#include "js/JSClass.hpp"

namespace NativeJS
{
	namespace JS
	{
		class App : public ObjectWrapper
		{
		public:
			App(const Env& env);

			virtual void initializeProps();

			bool isInitialized() const;

			JS_METHOD_DECL(onLoad);
			JS_METHOD_DECL(onTick);
			JS_METHOD_DECL(onQuit);

		private:
			bool isInitialized_;
		};

		class AppClass : public Class
		{
			JS_CLASS_BODY(AppClass);

		private:
			JS_CLASS_METHOD(onInit);
			JS_CLASS_METHOD(onGet);
		};
	}
}