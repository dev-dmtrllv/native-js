import NativeJS from "native-js";
import App from "./App";

const main: Entry = async (args) =>
{
	await NativeJS.App.initialize(App, args);
};

export default main;