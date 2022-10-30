import { LinkProps } from "./components/Link";
import { Footer } from "./layout/Footer";
import { Navbar } from "./layout/NavBar";

export class App extends UI.Component
{
	private static links: Required<LinkProps>[] = [
		{
			to: "home",
			text: "Home"
		},
		{
			to: "test",
			text: "Test"
		}
	];

	protected initializeStyle(): UI.Style | Promise<UI.Style>
	{
		return {

		};
	}

	protected initializeState(props: Readonly<{}>): {} | Promise<{}>
	{
		return {

		};
	}

	public render(props: Readonly<{}>, state: Readonly<{}>)
	{
		return (
			<view>
				<Navbar links={App.links} />
				<view>
					Hello App :D
				</view>
				<Footer links={App.links} />
			</view>
		)
	}

}