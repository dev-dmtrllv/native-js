import NativeJS from "native-js";

export class Window extends NativeJS.Window
{
	protected onClose(e: NativeJS.ICancelableEvent): void
	{
		console.log("Window close!");
	}

	protected onClosed(): void
	{
		console.log("Window closed!");
	}
}