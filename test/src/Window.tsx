import NativeJS from "native-js";
import { App } from "./ui/App";

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

	protected onLoad(e: NativeJS.IEvent): void
	{
		console.log("Window load!");
		console.log(this.render());
	}

	protected render()
	{
		return (
			<App />
		);
	}
}