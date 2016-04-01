/*
 * The tuner class - an active component friend
 * to the simulation class, manages by changing
 * parameters depending on received frame time 
 * information.
 */
#pragma once 
#include "simulation.h"


class Tuner
{
public:
	Tuner()	{}
	~Tuner() {}

	void initialize(Simulation*, int, int, cl_device_type);
	void start();
	bool report(double);
	bool tune();
	bool benchmark();

	//Change functions
	static float y_CPU_res(int x)
	{ return 0.000002347*x*x*x + 0.0112172; }
	static float y_GPU_res(int x)
	{ return 0.000000021*x*x*x + 0.00961385; }
	static float y_CPU_prec(int x)
	{ return 0.00129959*x - 0.00157226;}
	static float y_GPU_prec(int x)
	{ return 0.000376968*x - 0.00059281;}

private:
	Simulation* simulation;
	cl_device_type deviceType;

	float resolution, precision;
  
	int historySize;
	double averageLatest;
	double desiredFrameTime;
	double tuneStart, tuneTime;
};
