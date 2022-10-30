import NativeJS from "native-js";
import { Window } from "./Window";

export default class App extends NativeJS.App
{
	protected async onLoad(args: NativeJS.AppArgs)
	{
		// Create a window
		const w = new Window("Window 1 :D");
		w.show();
	}

	protected async onQuit(e: NativeJS.QuitEvent)
	{
		console.log("Quiting app...");
	}
}