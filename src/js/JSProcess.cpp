#include "framework.hpp"
#include "js/JSProcess.hpp"
#include "js/Env.hpp"
#include "js/JSObject.hpp"
#include "js/JSUtils.hpp"
#include "App.hpp"

namespace NativeJS::JS::Process
{
	void expose(const Env& env, Object& global)
	{
		std::vector<const char*> args = env.app().getAppArgs();

		Object process(env);
		process.set("args", mapStringArray(env, args), v8::PropertyAttribute::ReadOnly);
		global.set("process", *process);
	}
}