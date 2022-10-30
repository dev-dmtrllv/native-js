declare namespace JSX
{
	interface IntrinsicElements
	{
		view: UI.BaseProps & {};
		link: UI.BaseProps & {
			to: string;
			text?: string;
		};
	}

	interface Element extends UI.Element<any, any> {  }

	interface ElementClass extends UI.Component<any, any> {  }
}

declare namespace UI
{
	interface BaseProps
	{
		style?: Style;
	}

	interface Element<P extends {} = {}, T extends string | UI.FunctionComponent = string | UI.FunctionComponent>
	{
		type: T;
		props: P;
	}

	type FunctionComponent<P extends {} = {}> = (props: P) => JSX.Element; 

	type Style = {
			
	};

	abstract class Component<Props extends {} = {}, State extends {} = {}>
	{
		public constructor(props: Props);
		
		protected abstract initializeStyle(): Style | Promise<Style>;
		protected abstract initializeState(props: Readonly<Props>): State | Promise<State>;

		protected readonly setStyle: (style: Partial<Style>, onUpdated?: (...args: any[]) => any) => any;
		protected readonly setState: (state: Partial<State>, onUpdated?: (...args: any[]) => any) => any;

		protected onMount(): any;
		protected onUnmount(): any;

		public abstract render(props: Readonly<Props>, state: Readonly<State>);
	}

	const createElement: (...args: any[]) => JSX.Element;
}