#pragma once

#include "framework.hpp"
#include "StrongAtomic.hpp"

#define WORK_EVENT_CLASS(__NAME__, __EVENT_TYPE__) class __NAME__ : public WorkEvent \
{ \
public: \
	__NAME__(Worker& worker, WorkCallback work, ResolverCallback resolver, void* data) : \
		WorkEvent(__EVENT_TYPE__, worker, work, resolver, data) \
	{ } \
	virtual ~__NAME__() { } \
};

namespace NativeJS
{
	class Worker;

	namespace JS
	{
		class Env;
	}

	class Event
	{
	public:
		enum class Type
		{
			Unknown,
			Async,
			Native,
			Blocking,
			Message,
			Timeout,
			Terminate
		};

		enum class Status
		{
			Pending,
			Processing,
			Canceled,
			Done,
			Error
		};

	private:
		static Event _terminateEvent;
	public:
		static inline Event* getTerminateEvent() { return &_terminateEvent; }

		Event(Type type, void* data = nullptr);
		virtual ~Event() { };

		template<typename T>
		T& as() { return *(static_cast<T*>(this)); }

		bool trySetStatus(const Status& status);
		bool trySetStatus(Status&& status);
		virtual bool cancel();

		Status status() const;
		Type type() const;

		template<typename T = void>
		inline T* data() const { return static_cast<T*>(data_); }

		inline void setData(void* data) { data_ = data; }

	protected:
		const Type type_;
		StrongAtomic<Status> status_;
		size_t index_;
		void* data_;

		friend class EventAllocator;
	};

	class WorkEvent;

	using WorkCallback = void(*)(Event*);
	using ResolverCallback = void(*)(const WorkEvent&);

	class WorkEvent : public Event
	{
	public:
		WorkEvent(Event::Type type, Worker& worker, WorkCallback work, ResolverCallback resolver, void* data);

		virtual ~WorkEvent() = 0;

		Worker& worker() const { return worker_; }
		inline void resolve() const { resolver_(*this); }
		void resolvePromise(v8::Local<v8::Value> val = v8::Local<v8::Value>()) const;
		v8::Local<v8::Promise> promise() const;

	private:
		Worker& worker_;
		WorkCallback work_;
		ResolverCallback resolver_;
		v8::Persistent<v8::Promise::Resolver> promiseResolver_;

		friend class AsyncWorker;
		friend class App;
	};

	WORK_EVENT_CLASS(AsyncEvent, Event::Type::Async);

	class BlockingEvent : public Event
	{
	public:
		BlockingEvent(Worker& worker, WorkCallback work, void* data = nullptr);

		inline Worker& worker() const { return worker_; }
		inline bool isDone() const { return done_.load(std::memory_order::acquire); }

	private:
		Worker& worker_;
		WorkCallback work_;
		std::atomic<bool> done_;

		friend class AsyncWorker;
		friend class App;
	};

	class MessageEvent : public Event
	{
	public:
		MessageEvent(Worker* sender, Worker* receiver, std::string&& message);
		/**
		 * @brief
		 *
		 * @return Worker& the worker that owns the event
		 */
		inline Worker& sender() const { return *sender_; }
		inline Worker& receiver() const { return *receiver_; }

		inline const std::string& message() const { return message_; }
		v8::Local<v8::Promise> promise() const;
		void resolvePromise() const;

	private:
		Worker* sender_;
		Worker* receiver_;
		std::string message_;
		v8::Persistent<v8::Promise::Resolver> promiseResolver_;
	};

#ifdef _WINDOWS
	struct OSEvent
	{
		HWND hwnd;
		UINT uMsg;
		WPARAM wParam;
		LPARAM lParam;
	};
#endif

	class NativeEvent : public Event
	{
	public:
		NativeEvent(const OSEvent& osEvent, size_t sendCount);
		virtual ~NativeEvent();

		const OSEvent& event() const;

		/**
		 * @returns true if the callback was the last which processed the event
		 */
		template<typename Callback>
		bool process(Callback callback)
		{
			if (status() != Status::Canceled)
			{
				callback(nativeEvent_);
			}

			const size_t c = (*finishCount_).fetch_add(1) + 1;
			return c == sendCount_;
		}

	private:
		const OSEvent nativeEvent_;
		const size_t sendCount_;
		StrongAtomic<size_t> finishCount_;
	};

	class TimeoutEvent : public Event
	{
	public:
		enum class Type
		{
			INIT,
			CANCEL,
			RESET,
		};

		TimeoutEvent(const JS::Env& env, const size_t timeoutIndex, Type type);
		TimeoutEvent(const JS::Env& env, const size_t timeoutIndex, const std::chrono::milliseconds resolveTime, const size_t duration, const bool loop);

		const Type type;
		const JS::Env& env;
		const size_t timeoutIndex;
		const std::chrono::milliseconds resolveTime;
		const size_t duration;
		const bool loop;
	};
}