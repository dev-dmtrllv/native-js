#include "framework.hpp"
#include "js/JSEnvClasses.hpp"
#include "js/Env.hpp"

namespace NativeJS::JS
{
	EnvClasses::EnvClasses(const Env& env) :
		windowClass(env),
		appClass(env),
		eventClass(env),
		workerClass(env),
		timeoutClass(env),
		isInitialized_(false)
	{ }

	void EnvClasses::initialize()
	{
		if (!isInitialized_)
		{
			windowClass.initialize();
			appClass.initialize();
			eventClass.initialize();
			workerClass.initialize();
			timeoutClass.initialize();

			isInitialized_ = true;
		}
	}
}