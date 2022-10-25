declare module "native-js"
{
	export namespace NativeJS
	{
		interface IEvent
		{
			
		}

		interface ICancelableEvent extends IEvent
		{
			get isCanceled(): boolean;
			readonly cancel: () => void;
		}
	}
}