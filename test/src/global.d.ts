declare module "native-js"
{
	namespace NativeJS
	{
		interface IAppHolder
		{
			readonly app: import("./App").default;
		}
	}
}