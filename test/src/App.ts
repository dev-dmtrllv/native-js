import NativeJS from "native-js";
import { Window } from "./Window";

export default class App extends NativeJS.App
{
	protected async onLoad(args: NativeJS.AppArgs)
	{
		// Create a window
		const w = new Window("Window 1 :D");
		w.show();

		let i = 0;

		const interval = setInterval(() => 
		{
			console.log(i++);
		}, 500);

		// const interval2 = setInterval(() => 
		// {
		// 	console.log(i++);
		// }, 711);

		// setTimeout(() => 
		// {
		// 	interval.cancel();
		// 	interval2.cancel();
		// }, 3000);
	}

	protected async onQuit(e: NativeJS.QuitEvent)
	{
		console.log("Quiting app...");
	}
}