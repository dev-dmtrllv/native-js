declare namespace JSX
{
	interface IntrinsicElements
	{
		foo: any;
	}

	interface Element
	{

	}

	interface ElementClass
	{
		render: any;
	}
}

declare namespace UI
{
	const createElement: (...args: any) => JSX.Element;
}