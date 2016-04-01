#include "main.h"
#include "simulation.h"
#include "fluid.h"
#include "camera.h"
#include "graphics.h"
#include "File.h"

// OpenCL instance
OpenCL opencl;

// Main program
Main mainProgram;
cl_device_type deviceType;
bool sequential;
bool render;

//Logs
Log profileLog("profile.log");
Log framesLog("frames.log");
bool logging;

// Camera
Camera camera;

//General error checking function
inline void checkErr(cl_int err, const char * name)
{
        if(err != CL_SUCCESS)
        {
                std::cerr << "ERROR: " << name
                        << " (" << err << ")" << std::endl;
                exit(EXIT_FAILURE);
        }
}

//GLFW error function
static void glfwError(int error, const char* description)
{
	cout<<error<<": "<<description<<endl;
}

//Resized window - inform OpenGL
static void globalResize(GLFWwindow* window, int width, int height)
{
	mainProgram.g.resize(width, height);
}

//Keys
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	//Navigation
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if(key == GLFW_KEY_W && action == GLFW_PRESS)
		camera.hold(Camera::FORWARD);
	if(key == GLFW_KEY_A && action == GLFW_PRESS)
		camera.hold(Camera::LEFT);
	if(key == GLFW_KEY_S && action == GLFW_PRESS)
		camera.hold(Camera::BACKWARD);
	if(key == GLFW_KEY_D && action == GLFW_PRESS)
		camera.hold(Camera::RIGHT);
	if(key == GLFW_KEY_Q && action == GLFW_PRESS)
		camera.hold(Camera::DOWN);
	if(key == GLFW_KEY_E && action == GLFW_PRESS)
		camera.hold(Camera::UP);

	//Looking
	if(key == GLFW_KEY_LEFT && action == GLFW_PRESS)
		camera.hold(Camera::LOOK_LEFT);
	if(key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
		camera.hold(Camera::LOOK_RIGHT);
	if(key == GLFW_KEY_UP && action == GLFW_PRESS)
		camera.hold(Camera::LOOK_UP);
	if(key == GLFW_KEY_DOWN && action == GLFW_PRESS)
		camera.hold(Camera::LOOK_DOWN);

	if(action == GLFW_RELEASE)
		camera.hold(Camera::STILL);

	//Volume modification
	if(key == GLFW_KEY_F && action == GLFW_PRESS)
		mainProgram.simulation->addFluid();
	if(key == GLFW_KEY_G && action == GLFW_PRESS)
		mainProgram.simulation->addForce();
	if(key == GLFW_KEY_R && action == GLFW_PRESS)
		mainProgram.simulation->reset();
	if(key == GLFW_KEY_C && action == GLFW_PRESS)
		mainProgram.simulation->clearEffects();

	if(key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
	{
		mainProgram.simulation->resize(mainProgram.simulation->getN() + 1);
		if(render)
		{
			mainProgram.rayCaster.setN(mainProgram.simulation->getN());
			mainProgram.rayCaster.setVolume(mainProgram.simulation->getOutputVolume());
		}
	}
	if(key == GLFW_KEY_MINUS && action == GLFW_PRESS)
	{
		mainProgram.simulation->resize(mainProgram.simulation->getN() - 1);
		if(render)
		{
			mainProgram.rayCaster.setN(mainProgram.simulation->getN());
			mainProgram.rayCaster.setVolume(mainProgram.simulation->getOutputVolume());
		}
	}

	//Output debug info (not used)
	if(key == GLFW_KEY_ENTER && action == GLFW_PRESS)
		mainProgram.simulation->debug();
}

//TODO: camera look with mouse
static void mouseCallback(GLFWwindow* window, int button, int action, int mods)
{
}

/*
 * Sets everything up, including the OpenCL context
 */
void Main::initialize(int fps)
{
	targetFPS = fps;

	//Initialize OpenGL, set up a GLFW application (the window)
	if(!render)
		cout<<"No rendering will take place"<<endl;

	glfwSetErrorCallback(glfwError);
	if(render)
	{
		glfwInit();
		window = glfwCreateWindow(640, 480, "Fluid sim", NULL, NULL);
		glfwMakeContextCurrent(window);
		
		g.initialize(window);

		glfwSetKeyCallback(window, keyCallback);
		glfwSetMouseButtonCallback (window, mouseCallback);
		glfwSetWindowSizeCallback(window, globalResize);
	}


	//Initialize OpenCL context
	vector<cl::Platform> platformList;
	cl::Platform::get(&platformList);
	checkErr(platformList.size()!=0 ? CL_SUCCESS : -1, "cl::Platform::get");
	std::string platformVendor;
	platformList[0].getInfo((cl_platform_info)CL_PLATFORM_VENDOR, &platformVendor);
	std::cerr << "Platform is by: " << platformVendor << "\n";
	platformList[0].getInfo((cl_platform_info)CL_PLATFORM_VERSION, &platformVendor);
	std::cerr << platformVendor << "\n";
			
	cl_context_properties cprops[3] =
	{CL_CONTEXT_PLATFORM, (cl_context_properties)(platformList[0])(), 0};
	opencl.context = new cl::Context(deviceType, cprops, NULL, NULL, &opencl.err);
	opencl.checkErr("Conext::Context()");

	//Find devices
	vector<cl::Device> devices;
	devices = opencl.context->getInfo<CL_CONTEXT_DEVICES>();
	checkErr(devices.size() > 0 ? CL_SUCCESS : -1, "devices.size() > 0");

	//Compile CL program from sources in fluid.cl, raycast.cl and fluid_sequential.cl
	File raycastSourceFile("raycaster.cl");
	File fluidSourceFile("fluid.cl");
	File seqFluidSourceFile("fluid_sequential.cl");
	char* raycastSource = raycastSourceFile.ReadAll();
	char* fluidSource = fluidSourceFile.ReadAll();
	char* seqFluidSource = seqFluidSourceFile.ReadAll();

	cl::Program::Sources source;
	source.push_back(std::make_pair(raycastSource, raycastSourceFile.GetLength()));
	source.push_back(std::make_pair(fluidSource, fluidSourceFile.GetLength()));
	source.push_back(std::make_pair(seqFluidSource, seqFluidSourceFile.GetLength()));
	opencl.program = new cl::Program(*opencl.context, source);
	opencl.err = opencl.program->build(devices,"");
	cout<<opencl.program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
	opencl.checkErr("Program::build()");

	//Make queue
        opencl.queue = new cl::CommandQueue(*opencl.context, devices[0], logging ? CL_QUEUE_PROFILING_ENABLE : 0, &opencl.err);
        opencl.checkErr("CommandQueue::CommandQueue()");

	// Simulation and raycasting components
	if(sequential)
		simulation = new SequentialSimulation();
	else
		simulation = new ParallelSimulation();
	simulation->initialize(20);
	tuner.initialize(simulation, 10, targetFPS, deviceType);

	if(render)
		rayCaster.initialize(simulation->getN(), simulation->getOutputVolume());
}

//Timer - should be precise enough, at least on UNIX
inline double highResTime()
{
	return (double)clock() / CLOCKS_PER_SEC;
}

//Main loop
void Main::run()
{
	int frames = 0, frameAtBase = 0;
	double baseTime = highResTime();
	while (true)
	{
		if(render && glfwWindowShouldClose(window)) //process close only if rendering
			break;
		if(logging)
			profileLog<<"Frame "<<frames<<endl;
		double time = highResTime();

		if(render)
		{
			glfwPollEvents();

			//Set camera for raycaster
			camera.update();
			rayCaster.setCamera(camera);
		}

		//Simulate one step using OpenCL
		simulation->step();

		if(render)
		{
			//Get image from raycaster, pass it to OpenGL
			rayCaster.shoot();
			g.updateTexture(rayCaster.getTexture());
	 
			// Render the raycasted texture on a quad
			g.renderTexture();
		}


		double delta;
		delta = highResTime() - time; //current time delta
		framesLog<<frames<<" "<<delta<<endl; //frame time log

		/* Limit framerate to targetFPS (turned off, let autotuner handle)
		while(true)
		{
			delta = highResTime() - time;
			double rest = (1.0/targetFPS) - delta;
			if(rest <= 0)
				break;
			usleep(rest / 1000000); //sleep off the rest of the frame
		}
		*/

		//A major change is that of the resolution, in which case
		//the raycaster also needs to know.
		bool changed = tuner.report(delta);
		if(render && changed)
		{
			rayCaster.setN(simulation->getN());
			rayCaster.setVolume(simulation->getOutputVolume());
		}

		//FPS counter - update once a second
		frames++;
		if(time - baseTime > 1)
		{
			cout<<"FPS "<<frames - frameAtBase<<endl;
			if(logging)
			{
				framesLog<<"FPS "<<frames - frameAtBase<<endl;		
				framesLog.Commit();
				profileLog.Commit();
			}
			frameAtBase = frames;
			baseTime = highResTime();
		}
	}
}

int main(int argc, char* argv[])
{
	cout<<"Fluid simulation"<<endl;

	// Defaults
	logging = true;
	sequential = false;
	render = true;
	deviceType = CL_DEVICE_TYPE_ALL;
	int targetFPS = 20;

	// Parse arguments
	for(int i = 0; i < argc; i++)
	{
		if(strcmp(argv[i], "-nolog") == 0)
			logging = false;
		else if(strcmp(argv[i], "-cpu") == 0)
			deviceType = CL_DEVICE_TYPE_CPU;
		else if(strcmp(argv[i], "-gpu") == 0)
			deviceType = CL_DEVICE_TYPE_GPU;
		else if(strcmp(argv[i], "-sequential") == 0)
			sequential = true;
		else if(strcmp(argv[i], "-fps") == 0)
			targetFPS = atoi(argv[i+1]);
		else if(strcmp(argv[i], "-norender") == 0)
			render = false;
	}

	//Start simulating!
	mainProgram.initialize(targetFPS);
	mainProgram.run();
}
