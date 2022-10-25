/// <reference path="./Worker.d.ts" />
/// <reference path="./index.d.ts" />

declare type ProcessArgs = ReadonlyArray<string>;

interface Process
{
	args: ProcessArgs;
}

declare const process: Process;

declare interface Entry
{
	(args: ProcessArgs): void;
}

declare interface WorkerEntry
{
	(worker: MainWorker, args: ProcessArgs): void;
}