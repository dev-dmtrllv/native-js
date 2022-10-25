#include "framework.hpp"
#include "js/JSEvent.hpp"
#include "js/Env.hpp"
#include "Event.hpp"

namespace NativeJS::JS
{
	JS_CLASS_METHOD_IMPL(EventClass::ctor)
	{
		const size_t l = args.Length();
		if(l > 0)
		{
			if(args[0]->IsExternal())
			{
				args.This()->SetInternalField(0, args[0]);
			}
		}
		else
		{
			args.This()->SetInternalField(0, v8::Local<v8::External>());
			
		}
	}	

	JS_CLASS_METHOD_IMPL(EventClass::cancel)
	{
		auto v = args.This()->GetInternalField(0);
		if(!v.IsEmpty())
		{
			Event* e = static_cast<Event*>(v.As<v8::External>()->Value());
			e->cancel();
		}
	}

	JS_CREATE_CLASS(EventClass)
	{
		builder.setConstructor(ctor, this);
		builder.setInternalFieldCount(1);
		builder.setMethod("cancel", cancel, this);
	}
}