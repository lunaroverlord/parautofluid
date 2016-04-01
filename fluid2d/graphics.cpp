#include "graphics.h"

void Graphics::initialize()
{
	//Set up graphics
	glfwGetFramebufferSize(window, &width, &height);
}

void Graphics::renderDensity(int N, float* density)
{
	float ratio = width / (float) height;

	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	//Orthographic projection for 2D
	glOrtho(0, 1.0, 0, 1.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	float xOffset = 0.0, yOffset = 0.0;
	glPointSize(width / (N + 2));
	glBegin(GL_POINTS);
	for(int j = 0; j < N + 2; j++)
		for(int i = 0; i < N + 2; i++)
		{
			float intensity = density[IX(i, j)];
			glColor3f(intensity, intensity, intensity);
			glVertex3f((float)i/(N+2) + xOffset,
					(float)j/(N+2) + yOffset, 0);
		}
	glEnd();

	glfwSwapBuffers(window);
}
