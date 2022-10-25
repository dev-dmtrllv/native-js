/// <reference path="./Color.d.ts" />
/// <reference path="./Event.d.ts" />

declare module "native-js"
{
	namespace NativeJS
	{
		export type WindowOptions = {
			backgroundColor?: Color;
		};

		export class Window
		{
			public static readonly create: <T extends Window>(type: new (title: string, options?: WindowOptions) => T, title: string, options?: WindowOptions) => Promise<T>;

			public constructor(title: string, options?: WindowOptions);
		
			public show(): Promise<void>;
			public maximize(): Promise<void>;
			public minimize(): Promise<void>;

			protected onLoad(e: IEvent): void;
			protected onClose(e: ICancelableEvent): void;
			protected onClosed(e: IEvent): void;
		}
	}

	export const Window: typeof NativeJS.Window;
	export type Window = NativeJS.Window;
}