export const Link = (props: LinkProps) =>
{
	return (
		<link to={props.to} text={props.text} />
	);
}

export type LinkProps = {
	to: string;
	text?: string;	
};