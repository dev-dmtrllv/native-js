declare class Timeout
{
	public constructor(callback: TimeoutCallback, timeout: number | BigInt, loop?: boolean);

	public reset(callback: TimeoutCallback, timeout: number | BigInt, loop?: boolean);
	public cancel();
}

type TimeoutCallback = (timeout: Timeout) => any;