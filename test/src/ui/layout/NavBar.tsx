import { Link, LinkProps } from "../components/Link";

export const Navbar = (props: NavbarProps) =>
{
	return (
		<view>
			Navbar
			<view style={{  }}>
				{props.links.map(link => <Link {...link} />)}
			</view>
		</view>
	);
}

type NavbarProps = {
	links: LinkProps[];
};