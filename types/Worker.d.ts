/// <reference path="./Event.d.ts" />
declare class Worker
{
	public static getParentWorker(): Worker | null;

	public constructor(entry: string);

	public on(eventType: string, callback: () => any): void;
	public send(msg: string): void;
	public terminate(): Promise<void>;
}

type MainWorker = Omit<Worker, "terminate">;