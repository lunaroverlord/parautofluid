#include "tuner.h"
#include "main.h"


void Tuner::initialize(Simulation* sim, int c, int fps, cl_device_type devType)
{
	simulation = sim;
	historySize = c;
	averageLatest = 0;
	tuneTime = 0.2;
	desiredFrameTime = 1.0 / fps;
	tuneStart = highResTime();
	deviceType = devType;

	//Locally kept parameters
	resolution = simulation->N;
	precision = simulation->solverSteps;
}

/*
 * Any setting up code for more advanced versions of the tuner
 */
void Tuner::start()
{
}

//Frame time feeding function, the simulation reports its
//frame times to this.
bool Tuner::report(double frameTime)
{
	//return benchmark();

	//Actual tuning
	if(averageLatest == 0)
		averageLatest = frameTime * historySize;
	else
		averageLatest = ((averageLatest * (historySize - 1)) + frameTime) / historySize;

	if(highResTime() - tuneStart > tuneTime)
	{
		tuneStart = highResTime();
		return tune();
	}
	return false;
}

/*
 * The decision making function - looks at the average frame times
 * and uses the change functions to decide which paramter to nudge.
 *
 * It uses a least-change strategy, changing the weakest influence 
 * parameter first, before moving the stronger one.
 */
bool Tuner::tune()
{
	double difference = averageLatest - desiredFrameTime;
	cout<<"TUNER: difference = "<<difference;

	bool changed = false;
	//Positive difference = bad, negative = good
	if(difference > 0.01 && simulation->solverSteps > 1 && simulation->N > 4)
	{
		//how much a resolution change should cause a precision change?
		float dydx = 
		(deviceType == CL_DEVICE_TYPE_CPU)
		?
			(y_CPU_res(simulation->N) - y_CPU_res(simulation->N - 1))
			/
			(y_CPU_prec(simulation->solverSteps) - y_CPU_prec(simulation->solverSteps - 1))
		:
			(y_GPU_res(simulation->N) - y_GPU_res(simulation->N - 1))
			/
			(y_GPU_prec(simulation->solverSteps) - y_GPU_prec(simulation->solverSteps - 1));

		if(dydx < 1)
		{
			resolution--;
			precision -= dydx;
		}
		else
		{
			precision--;
			resolution -= 1.0/dydx;
		}

		cout<<" (changing down) ";
	}
	else if(difference < 0)
	{
		//how much a resolution change should cause a precision change?
		float dydx = 
		(deviceType == CL_DEVICE_TYPE_CPU)
		?
			(y_CPU_res(simulation->N + 1) - y_CPU_res(simulation->N))
			/
			(y_CPU_prec(simulation->solverSteps + 1) - y_CPU_prec(simulation->solverSteps))
		:
			(y_GPU_res(simulation->N + 1) - y_GPU_res(simulation->N))
			/
			(y_GPU_prec(simulation->solverSteps + 1) - y_GPU_prec(simulation->solverSteps));

		if(dydx < 1)
		{
			resolution++;
			precision += dydx;
		}
		else
		{
			precision++;
			resolution += 1.0/dydx;
		}

		cout<<" (changing up) ";
	}

	if(int(resolution) != simulation->N)
	{
		simulation->resize(int(resolution));
		changed = true;
	}
	simulation->solverSteps = int(precision);
	cout<<" R/P = "<<simulation->N<<"/"<<simulation->solverSteps<<endl;

	return changed;
}

/*
 * Benchmarking response from the tuner, not used in actual simulation
 */
bool Tuner::benchmark()
{
	static int count = 0;

	//Choose one parameter to change every 20 frames
	if(count % 20 == 0 && count != 0)
	{
		//Resolution change (solvers fixed at 20 iterations)
		simulation->resize(1);
		return true;
		
		
		//Solver precision change (resolution fixed at 20x20x20)
		simulation->solverSteps++;
		cout<<"solver steps: "<<simulation->solverSteps<<endl;
		return false;
	}

	count++;
	
	return false;
}
