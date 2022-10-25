#pragma once

#include "js/JSClass.hpp"

namespace NativeJS
{
	namespace JS
	{
		class Worker : public ObjectWrapper
		{
		public:
			Worker(const Env& env);
			virtual ~Worker();

			virtual void initializeProps();

			JS_METHOD_DECL(send);
			JS_METHOD_DECL(terminate);

			void emitMessage(const std::string& str);

		private:
			v8::Persistent<v8::Object> listeners_;
		};

		class WorkerClass : public Class
		{
			JS_CLASS_BODY(WorkerClass);

		private:
			JS_CLASS_METHOD(getParentWorker);
			JS_CLASS_METHOD(ctor);
			JS_CLASS_METHOD(terminate);
			JS_CLASS_METHOD(send);
			JS_CLASS_METHOD(onMessage);
		};
	}
}