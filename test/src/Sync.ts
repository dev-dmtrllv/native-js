export class Sync<T>
{
	public static readonly create = <T>(initialValue: T, syncCallback: SyncCallback<T>) => new Sync<T>(initialValue, syncCallback);

	private constructor(initialValue: T, syncCallback: SyncCallback<T>)
	{
		this.value_ = initialValue;
		this.syncCallback_ = syncCallback;
	}

	public readonly get: () => Readonly<T>;

	public readonly update: (value: Partial<T>) => Sync<T>;
	public readonly tryUpdate: (value: Partial<T>) => boolean;
	public readonly updateAsync: (value: Partial<T>) => Promise<Sync<T>>;

	private value_: T;
	private syncCallback_: SyncCallback<T>;
}

type SyncCallback<T> = (self: T, syncValue: Partial<T>) => T;