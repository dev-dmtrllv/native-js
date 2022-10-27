const main: WorkerEntry = async (worker) =>
{
	worker.on("test", () => 
	{
		console.log("get test from main");
	});

	new Timeout(() => 
	{
		new Timeout(() => 
		{
			worker.send("test");
		}, 1000, true);
	}, 500);
};

export default main;