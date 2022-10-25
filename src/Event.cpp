#include "framework.hpp"
#include "Event.hpp"
#include "Worker.hpp"
#include "js/Env.hpp"

namespace NativeJS
{
	Event Event::_terminateEvent = Event(Event::Type::Terminate);

	Event::Event(Event::Type type, void* data) :
		type_(type),
		status_(Status::Pending),
		data_(data)
	{

	}

	bool Event::trySetStatus(const Status& status)
	{
		Status expected = this->status();
		return (*status_).compare_exchange_weak(expected, status, std::memory_order_acq_rel);
	}

	bool Event::trySetStatus(Status&& status)
	{
		Status expected = this->status();
		return (*status_).compare_exchange_weak(expected, status, std::memory_order_acq_rel);
	}

	Event::Status Event::status() const
	{
		return status_.get();
	}

	Event::Type Event::type() const
	{
		return type_;
	}

	bool Event::cancel()
	{
		do
		{
			Status s = status();
			if (s == Status::Done || s == Status::Error || s == Status::Canceled)
				return false;
		} while (!trySetStatus(Status::Canceled));
		return true;
	}

	void WorkEvent::resolvePromise(v8::Local<v8::Value> val) const
	{
		const JS::Env& env = worker_.env();
		promiseResolver_.Get(env.isolate())->Resolve(env.context(), val.IsEmpty() ? v8::Undefined(env.isolate()).As<v8::Value>() : val).ToChecked();
	}

	v8::Local<v8::Promise> WorkEvent::promise() const
	{
		return promiseResolver_.Get(worker_.env().isolate())->GetPromise();
	}

	WorkEvent::WorkEvent(Event::Type type, Worker& worker, WorkCallback work, ResolverCallback resolver, void* data) :
		Event(type, data),
		worker_(worker),
		work_(work),
		resolver_(resolver),
		promiseResolver_(worker.env().isolate(), v8::Promise::Resolver::New(worker.env().context()).ToLocalChecked())
	{ }

	WorkEvent::~WorkEvent() { }

	BlockingEvent::BlockingEvent(Worker& worker, WorkCallback work, void* data) :
		Event(Event::Type::Blocking, data),
		worker_(worker),
		work_(work),
		done_(false)
	{ }

	MessageEvent::MessageEvent(Worker* sender, Worker* receiver, std::string&& message) :
		Event(Event::Type::Message),
		sender_(sender),
		receiver_(receiver),
		message_(message),
		promiseResolver_(sender->env().isolate(), v8::Promise::Resolver::New(sender->env().context()).ToLocalChecked())
	{ }


	v8::Local<v8::Promise> MessageEvent::promise() const
	{
		return promiseResolver_.Get(sender_->env().isolate())->GetPromise();
	}

	void MessageEvent::resolvePromise() const
	{
		promiseResolver_.Get(sender_->env().isolate())->Resolve(sender_->env().context(), v8::Undefined(sender_->env().isolate()));
	}

	NativeEvent::NativeEvent(const OSEvent& osEvent, size_t sendCount) :
		Event(Event::Type::Native),
		nativeEvent_(osEvent),
		sendCount_(sendCount)
	{ }

	NativeEvent::~NativeEvent() { }

	const OSEvent& NativeEvent::event() const
	{
		return nativeEvent_;
	}

	TimeoutEvent::TimeoutEvent(const JS::Env& env, const size_t timeoutIndex, const std::chrono::milliseconds resolveTime, const bool loop) :
		Event(Event::Type::Timeout),
		env(env),
		timeoutIndex(timeoutIndex),
		resolveTime(resolveTime),
		loop(loop)
	{ }
}