import { Link, LinkProps } from "../components/Link";

export const Footer = (props: FooterProps) =>
{
	return (
		<view>Footer
			<view style={{  }}>
				{props.links.map(link => <Link {...link} />)}
			</view>
		</view>
	);
}

type FooterProps = {
	links: LinkProps[];
};