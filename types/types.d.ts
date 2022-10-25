declare type AbstractClass<InstanceType = any, Args extends any[] = any[]> =  abstract new (...args: Args) => InstanceType;

declare type Class<InstanceType = any, Args extends any[] = any> = new (...args: Args) => InstanceType;