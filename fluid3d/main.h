/*
 * Main program that holds everything together
 * 
 * Refer to the section on Implementation to see the
 * different components the program has.
 */
#pragma once

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring> //for memset
#include <cmath>
#include <unistd.h>
using namespace std;

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <GL/gl.h>

/*
 * Backwards compatibility wiht OpenCL 1.1
 */
#ifdef opencl11
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/cl.h>
#undef CL_VERSION_1_2
#endif
#include <CL/cl.hpp>

#include "graphics.h"
#include "simulation.h"
#include "tuner.h"
#include "raycaster.h"

#include "Log.h"
extern Log profileLog;
extern Log framesLog;
extern bool logging;
extern bool render;

//Timer
double highResTime();

/*
 * Global singleton structure to hold the OpenCL state.
 * Has many convenient wrappers that make the simulation step function shorter.
 */
struct OpenCL
{
	// OpenCL system
	cl_int err;
	cl::Context* context;
	cl::CommandQueue* queue;
	cl::Program* program;
	cl::Event event;

	void enqueue(const cl::Kernel &kernel, const cl::NDRange &offset, const cl::NDRange &global, const cl::NDRange &local)
	{
		err = queue->enqueueNDRangeKernel(kernel, offset, global, local, NULL, &event);
		checkErr("Kernel enqueuing failed");

		if(logging) //Output log?
		{
			string kernelFunction;
			kernel.getInfo(CL_KERNEL_FUNCTION_NAME, &kernelFunction);
			wait();
			cl_ulong startTime, endTime;
			event.getProfilingInfo(CL_PROFILING_COMMAND_START, &startTime);
			event.getProfilingInfo(CL_PROFILING_COMMAND_END, &endTime);
			profileLog<<kernelFunction<<" "<<endTime - startTime<<endl;
		}
	}

	void wait()
	{
		event.wait();
	}

	//OpenCL error function
	void checkErr(const char* name)
	{
		if(err != CL_SUCCESS)
		{
			std::cerr << "ERROR: " << name
				<< " (" << err << ")" << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	void destroy()
	{
		delete context;
		delete queue;
		delete program;
	}
} extern opencl;


/*
 * Main program object
 */
class Main
{
public:
	Main()
	{
		window = NULL;
	}
	~Main()
	{
		opencl.destroy();
		delete simulation;
	}

	void initialize(int fps);
	void run();



	Graphics g; //OpenGL gateway
	Simulation *simulation;
	Tuner tuner;
	RayCaster rayCaster;
private:
	GLFWwindow* window;

	int targetFPS;
} extern mainProgram;

