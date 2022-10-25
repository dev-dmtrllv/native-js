import NativeJS from "native-js";
import { Window } from "./Window";

export default class App extends NativeJS.App
{
	protected async onLoad(args: NativeJS.AppArgs)
	{
		const w = new Window("Window 1 :D");
		w.show();

		const worker = new Worker(__dirname + "test.worker");

		worker.on("test", () => 
		{
			console.log("test sended from worker :)");
		});

		const timeout = new Timeout(() =>
		{
			console.log("Hello");
		}, 1000, true);
	}

	protected async onQuit(e: NativeJS.QuitEvent)
	{
		console.log("Quiting app...");
	}
}