#include "framework.hpp"
#include "js/JSUI.hpp"
#include "js/Env.hpp"
#include "js/JSObject.hpp"
#include "js/JSUtils.hpp"

namespace NativeJS::JS
{
	namespace UI
	{
		JS_CALLBACK(createElement)
		{
			Object jsx(env);
			jsx.set("type", args[0]);
			jsx.set("props", args[1]);
			jsx.set("children", args[2]);

			args.GetReturnValue().Set(*jsx);
		}

		class ComponentClass : public Class
		{
			public:
				ComponentClass(const Env& env) :
					Class(env)
				{  }

				~ComponentClass() {  }

			
		protected:
			virtual void create(const ClassBuilder& builder)
			{

			}
		};

		void expose(const Env& env, Object& global)
		{
			Object uiNamespace(env);
			uiNamespace.set("createElement", createElement);
			ComponentClass componentClass(env);
			componentClass.initialize();
			uiNamespace.set("Component", componentClass.getClass());
			global.set("UI", *uiNamespace);
		}
	}
}