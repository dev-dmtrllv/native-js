/// <reference path="./Event.d.ts" />
/// <reference path="./types.d.ts" />
/// <reference path="./global.d.ts" />

declare module "native-js"
{
	export namespace NativeJS
	{
		export interface QuitEvent extends ICancelableEvent { }

		export type AppArgs = readonly string[];

		export abstract class App<OnLoadArgs = AppArgs>
		{
			protected abstract onLoad(args: OnLoadArgs): void;
			protected onTick(): void;
			protected onQuit(e: QuitEvent): void;

			/**
			 * @arg args_0 Class<App>
			 * @arg args_1 AppLoadArgs<App>
			 */
			static readonly initialize: (...args: AppInitializeArgs) => Promise<AppType>;
			static readonly get: () => IAppHolder["app"];
		}

		type AppType = IAppHolder["app"];

		type AppInitializeArgs = AppType extends App<infer Args> ? [Class<AppType>, Args] : [Class<AppType>];
		type AppLoadArgs<T> = T extends App<infer AppArgs> ? AppArgs : never;
	}

	export const App: typeof NativeJS.App;
	export type App = NativeJS.App;
}