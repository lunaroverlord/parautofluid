#include "main.h"
#include "fluid.h"
#include "graphics.h"


//Data for controlling using user input
Vector densityTarget, velocityTarget, velocityAmount;

static void glfwError(int error, const char* description)
{
	cout<<error<<": "<<description<<endl;
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

static void mouseCallback(GLFWwindow* window, int button, int action, int mods)
{
	if(button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS)
	{
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		densityTarget.x = int(x);
		densityTarget.y = int(y);
	}
	//velocity
	if(button == GLFW_MOUSE_BUTTON_2 && action == GLFW_PRESS)
	{
		double x, y;
		static double lastX = 0, lastY = 0;
		glfwGetCursorPos(window, &x, &y);
		velocityTarget.x = int(x);
		velocityTarget.y = int(y);
		velocityAmount.x = (x - lastX) * 100;
		velocityAmount.y = (y - lastY) * 100;
		lastX = x;
		lastY = y;

	}
}


void output(int N, float* matrix)
{
	cout<<"densities: "<<endl;
	int size = (N+2)*(N+2);
	for(int i = 0; i < N + 2; i++)
	{
		for(int j = 0; j < N + 2; j++)
			cout<<fixed<<setprecision(2)<<matrix[IX(j,i)]<<"\t";
		cout<<endl;
	}
}

int main()
{
	//Set up a GLFW application
	glfwInit();
	GLFWwindow* window = glfwCreateWindow(640, 480, "Fluid sim", NULL, NULL);
	glfwMakeContextCurrent(window);
	
	Graphics g(window);
	g.initialize();

	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback (window, mouseCallback);
	glfwSetErrorCallback(glfwError);

	//Allocate grid
	int N = 100;
	int size = (N+2)*(N+2) * sizeof(float);

	//Control variables
	densityTarget.x = 0;
	densityTarget.y = 0;

	float *u = new float[size],
	      *v = new float[size],
	      *u_prev = new float[size],
	      *v_prev = new float[size],
	      *dens = new float[size],
	      *dens_prev = new float[size];

	memset(u, 0, size);
	memset(v, 0, size);
	memset(u_prev, 0, size);
	memset(v_prev, 0, size);
	memset(dens, 0, size);
	memset(dens_prev, 0, size);
	

	float dt = 0.1; //timestep
	float visc = 0.001;
	float diff = 0.0005; //dampening


	/*
	for(int i = 0; i < size; i++)
		u_prev[i] = 1;
	for(int i = 1; i < size; i++)
		v_prev[i] = 1;
		*/
	//dens_prev[IX(2, 2)] = 100;

	int times = 0;
	//Simulation
	while (!glfwWindowShouldClose(window))
	{
		if(densityTarget.x != 0)
		{
			int x = ((float)densityTarget.x / g.getWidth()) * N;
			int y = (1 - (float)densityTarget.y / g.getHeight()) * N;
			dens_prev[IX(x, y)] = 500;
			dens_prev[IX(x + 1, y)] = 500;
			dens_prev[IX(x, y + 1)] = 500;
			dens_prev[IX(x + 1, y + 1)] = 500;
			densityTarget.x = 0;
		}
		if(velocityTarget.x != 0)
		{
			int x = ((float)velocityTarget.x / g.getWidth()) * N;
			int y = (1 - (float)velocityTarget.y / g.getHeight()) * N;
			u_prev[IX(x, y)] = velocityAmount.x;
			u_prev[IX(x, y)] = velocityAmount.y;
			velocityTarget.x = 0;
		}

		//get_from_UI ( dens_prev, u_prev, v_prev );
		vel_step ( N, u, v, u_prev, v_prev, visc, dt );
		dens_step ( N, dens, dens_prev, u, v, diff, dt );
		g.renderDensity ( N, dens );


		memset(u_prev, 0, size);
		memset(v_prev, 0, size);
		memset(dens_prev, 0, size);

	
		//Hope for POSIX
//		usleep(1000 * 66); //15fps
		usleep(1000 * 33); //30fps

		//Output every 10th time
		/*
		if(times % 10 == 0)
			output(N, dens);
			*/
		times++;

		glfwPollEvents();
	}


	delete[] u;
	delete[] v;
	delete[] u_prev;
	delete[] v_prev;
	delete[] dens;
	delete[] dens_prev;
}
