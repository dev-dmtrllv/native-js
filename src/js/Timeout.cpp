#include "framework.hpp"
#include "js/Timeout.hpp"
#include "js/Env.hpp"
#include "App.hpp"

namespace NativeJS::JS
{
	const char* Timeout::RESOLVER_KEY = "resolver_";

	Timeout::Timeout(const Env& env, std::chrono::milliseconds resolveTime, const bool loop) : 
		ObjectWrapper(env),
		index_(0),
		resolveTime_(resolveTime),
		loop_(loop)
	{ }

	Timeout::~Timeout() {}

	bool Timeout::shouldResolve(std::chrono::milliseconds time)
	{
		return time >= resolveTime_; 
	}

	void Timeout::initializeProps()
	{
		loadMethod(resolve_, RESOLVER_KEY);
	}

	void Timeout::setIndex(size_t index)
	{
		index_ = index;
	}

	JS_METHOD_IMPL(Timeout::resolve);

	JS_CLASS_METHOD_IMPL(TimeoutClass::ctor)
	{
		const size_t l = args.Length();
		if (l < 2)
		{
			env.throwException("Not enough arguments!");
		}
		if (!args[0]->IsFunction())
		{
			env.throwException("First arguments must be a function!");
		}
		else if (!args[1]->IsNumber() && !args[1]->IsBigInt())
		{
			env.throwException("Second arguments must be a number!");
		}
		else
		{
			args.This()->Set(env.context(), string(env, Timeout::RESOLVER_KEY), args[0]);
			env.addTimeout(args[0].As<v8::Function>(), args[1], args.This(), l >= 3 ? args[2] : v8::Local<v8::Value>());
		}
	}

	void TimeoutClass::create(const ClassBuilder& builder)
	{
		builder.setConstructor(TimeoutClass::ctor);
		builder.setMethod(Timeout::RESOLVER_KEY);
	}
}