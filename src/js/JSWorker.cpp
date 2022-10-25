#include "framework.hpp"
#include "js/JSWorker.hpp"
#include "js/Env.hpp"
#include "App.hpp"
#include "Worker.hpp"
#include "Event.hpp"
#include "js/JSUtils.hpp"

namespace NativeJS
{
	namespace JS
	{
		Worker::Worker(const Env& env) : ObjectWrapper(env) { }
		Worker::~Worker() { }

		void Worker::initializeProps()
		{
			loadMethod(send_, "send");
			loadMethod(terminate_, "terminate");

			v8::Local<v8::Value> listenersVal;
			if (getFromObject(env_, value(), "listeners_", listenersVal) && listenersVal->IsObject())
			{
				listeners_.Reset(env_.isolate(), listenersVal.As<v8::Object>());
			}
			else
			{
				puts("could not set listeners!");
			}
		}

		void Worker::emitMessage(const std::string& message)
		{
			if (listeners_.IsEmpty())
			{
				puts("empty :(");
			}
			else
			{
				v8::Local<v8::Object> v = listeners_.Get(env_.isolate());
				v8::MaybeLocal<v8::Value> maybeListeners = getFromObject(env(), v, string(env(), message));
				if (maybeListeners.IsEmpty())
				{
					puts("empty 2 :(");
				}
				else
				{
					v8::Local<v8::Value> arrVal = maybeListeners.ToLocalChecked();
					if (arrVal->IsArray())
					{
						v8::Local<v8::Array> arr = arrVal.As<v8::Array>();
						const size_t l = arr->Length();

						for (size_t i = 0; i < l; i++)
						{
							v8::Local<v8::Function> fn = arr->Get(env_.context(), i).ToLocalChecked().As<v8::Function>();
							fn->Call(env_.context(), fn, 0, nullptr).ToLocalChecked();
						}
					}
					else
					{
						puts("no array :S");
					}
				}
			}

		}

		JS_METHOD_IMPL(Worker::send);
		JS_METHOD_IMPL(Worker::terminate);

		JS_CLASS_METHOD_IMPL(WorkerClass::ctor)
		{
			const int l = args.Length();

			if (l == 0)
			{
				env.throwException("Not enough arguments!");
			}
			else if (args[0]->IsExternal())
			{
				NativeJS::Worker* w = parseExternal<NativeJS::Worker>(env, args[0]);
				if (env.isSelfWorker(w))
				{
					puts("create self worker");
				}
				else
				{
					puts("create self worker (from parent)");
				}

				args.This()->Set(env.context(), string(env, "listeners_"), v8::Object::New(env.isolate()));
				args.This()->SetInternalField(0, args[0]);
			}
			else if (args[0]->IsString())
			{
				struct Info
				{
					std::string entry;
					NativeJS::Worker* worker = nullptr;
					NativeJS::Worker* parentWorker = nullptr;
				};

				Info info;
				info.entry = parseString(env, args[0]);
				info.parentWorker = std::addressof(env.worker());

				args.This()->Set(env.context(), string(env, "listeners_"), v8::Object::New(env.isolate()));

				env.doBlockingWork([](Event* event)
				{
					BlockingEvent* e = static_cast<BlockingEvent*>(event);
					Info* info = e->data<Info>();
					info->worker = e->worker().app().createWorker(std::move(info->entry), info->parentWorker);
				}, &info, true);

				if (info.worker != nullptr)
				{
					setInternalPointer(args, info.worker);
					env.addJsWorker(info.worker, args.This());
				}
				else
				{
					puts("Could not create worker!");
					env.throwException("Could not create worker!");
				}
			}
		}

		JS_CLASS_METHOD_IMPL(WorkerClass::terminate)
		{
			NativeJS::Worker* worker = nullptr;

			if (args.This()->GetInternalField(0)->IsExternal())
				worker = static_cast<NativeJS::Worker*>(args.This()->GetInternalField(0).As<v8::External>()->Value());

			if (env.isSelfWorker(worker))
			{
				v8::Local<v8::Promise::Resolver> resolver = v8::Promise::Resolver::New(env.context()).ToLocalChecked();
				resolver->Reject(env.context(), string(env, "Cannot terminate parent worker!"));
				args.GetReturnValue().Set(resolver->GetPromise());
			}
			else
			{
				v8::Local<v8::Promise> promise = env.doAsyncWork([](Event* event)
				{
					AsyncEvent* e = static_cast<AsyncEvent*>(event);
					NativeJS::Worker* worker = e->data<NativeJS::Worker>();
					int exitCode = 0;
					e->worker().app().destroyWorker(worker);
				}, nullptr, worker, true);

				args.GetReturnValue().Set(promise);
			}
		}

		JS_CLASS_METHOD_IMPL(WorkerClass::send)
		{
			const size_t l = args.Length();
			if (l == 0)
			{
				v8::Local<v8::Promise::Resolver> resolver = v8::Promise::Resolver::New(env.context()).ToLocalChecked();
				resolver->Reject(env.context(), string(env, "Not enough arguments!"));
				args.GetReturnValue().Set(resolver->GetPromise());
			}
			else if (args.This()->GetInternalField(0)->IsExternal())
			{
				if (args[0]->IsString())
				{
					NativeJS::Worker* worker = static_cast<NativeJS::Worker*>(args.This()->GetInternalField(0).As<v8::External>()->Value());
					std::string name = parseString(env, args[0]);
					args.GetReturnValue().Set(env.sendMessageToWorker(worker, std::move(name)));
				}
				else
				{
					v8::Local<v8::Promise::Resolver> resolver = v8::Promise::Resolver::New(env.context()).ToLocalChecked();
					resolver->Reject(env.context(), string(env, "First argument is not of type string!"));
					args.GetReturnValue().Set(resolver->GetPromise());
				}
			}
			else
			{
				v8::Local<v8::Promise::Resolver> resolver = v8::Promise::Resolver::New(env.context()).ToLocalChecked();
				resolver->Reject(env.context(), string(env, "Could not get native worker!"));
				args.GetReturnValue().Set(resolver->GetPromise());
			}
		}

		JS_CLASS_METHOD_IMPL(WorkerClass::getParentWorker)
		{
			args.GetReturnValue().Set(env.getJsWorker().value());
		}

		JS_CLASS_METHOD_IMPL(WorkerClass::onMessage)
		{
			if (args.Length() == 2)
			{
				if (!args[0]->IsString())
				{
					env.throwException("First argument is not of type string!");
				}
				else if (!args[1]->IsFunction())
				{
					env.throwException("Second argument is not a function!");
				}
				else
				{
					v8::Local<v8::Value> mapVal;

					if (getFromObject(env, args.This(), "listeners_", mapVal))
					{
						v8::Local<v8::Object> map = mapVal.As<v8::Object>();
						v8::Local<v8::Value> listenersVal;
						v8::Local<v8::Array> listeners;

						if (!getFromObject(env.context(), map, args[0], listenersVal) || !listenersVal->IsArray())
						{
							listeners = v8::Array::New(env.isolate());
							if (!map->Set(env.context(), args[0], listeners).ToChecked())
							{
								env.throwException("Could not set listeners array!");
							}
						}
						else
						{
							listeners = listenersVal.As<v8::Array>();
						}

						listeners->Set(env.context(), listeners->Length(), args[1]); // add the callback to the array
					}
					else
					{
						env.throwException("Could not get listeners map!");
					}
				}
			}
			else
			{
				env.throwException("Not enough arguments!");
			}
		}

		JS_CREATE_CLASS(WorkerClass)
		{
			builder.setStaticMethod("getParentWorker", getParentWorker);
			builder.setConstructor(ctor);
			builder.setMethod("terminate", terminate, v8::Local<v8::Value>(), v8::PropertyAttribute::ReadOnly);
			builder.setMethod("send", send, v8::Local<v8::Value>(), v8::PropertyAttribute::ReadOnly);
			builder.setMethod("on", onMessage, v8::Local<v8::Value>(), v8::PropertyAttribute::ReadOnly);
			builder.setInternalFieldCount(1);
		}
	}
}