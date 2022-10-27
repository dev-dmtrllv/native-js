declare class Timeout
{
	public constructor(callback: TimeoutCallback, timeout: number | BigInt, loop?: boolean);

	public reset(callback: TimeoutCallback, timeout: number | BigInt, loop?: boolean);
	public cancel();
}

declare const setTimeout: (callback: TimeoutCallback, timeout: number | BigInt) => Timeout;
declare const setInterval: (callback: TimeoutCallback, timeout: number | BigInt) => Timeout;

type TimeoutCallback = (timeout: Timeout) => any;