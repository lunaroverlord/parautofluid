#include "main.h"

//Separates drawing code from simulation code
class Graphics
{
public:
	Graphics(GLFWwindow* window) : window(window) {}
	~Graphics()
	{
		//GLFW destroy
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void initialize();
	void renderDensity(int N, float* density);

	int getWidth() { return width; }
	int getHeight() { return height; }
private:
	GLFWwindow* window;
	int width, height;
};
