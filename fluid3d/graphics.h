/*
 * Drawing the texture using OpenGL, fairly straightforward
 */
#pragma once

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <CL/cl.hpp>

#include <cmath>

class Graphics
{
public:
	Graphics() {window = NULL;}
	~Graphics()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void initialize(GLFWwindow*);
	void renderTexture();
	void updateTexture(unsigned char*);

	int getWidth() { return width; }
	int getHeight() { return height; }
	void resize(int w, int h)
	{
		width = w;
		height = h;
	}
private:
	GLFWwindow* window;
	GLuint texture;
	int width, height;
};
