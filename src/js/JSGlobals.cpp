#include "framework.hpp"
#include "js/Env.hpp"
#include "js/JSObject.hpp"
#include "js/JSConsole.hpp"
#include "js/JSProcess.hpp"
#include "js/JSUI.hpp"

namespace NativeJS::JS::JSGlobals
{
	void expose(const Env& env, Object& global)
	{
		auto& timeoutClass = env.getJsClasses().timeoutClass;
		JS::Console::expose(env, global);
		JS::Process::expose(env, global);
		JS::UI::expose(env, global);

		global.set("Worker", env.getJsClasses().workerClass.getClass(), v8::PropertyAttribute::ReadOnly);
		global.set("Timeout", timeoutClass.getClass(), v8::PropertyAttribute::ReadOnly);
		
		v8::Local<v8::External> externalTimeoutClass = v8::External::New(env.isolate(), const_cast<void*>(static_cast<const void*>(std::addressof(timeoutClass))));
		global.set("setInterval", timeoutClass.setIntervalWrapper, externalTimeoutClass);
		global.set("setTimeout", timeoutClass.setTimeoutWrapper, externalTimeoutClass);
		
	}
}