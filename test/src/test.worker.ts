const main: WorkerEntry = async (worker) =>
{
	const timeout = new Timeout(() => 
	{
		console.log("Hello from worker :D");
		worker.send("test");
	}, 1000, true);
};

export default main;