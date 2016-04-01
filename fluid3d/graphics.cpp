#include "graphics.h"
#include <iostream>

/*
 * Set up the texture and rendering environment
 */
void Graphics::initialize(GLFWwindow* w)
{
	window = w;
	glfwGetFramebufferSize(window, &width, &height);
	std::cout<<"Window: "<<width<<" * "<<height<<std::endl;

	//glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}

//New bitmap coming in?
void Graphics::updateTexture(unsigned char* pixels)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

void Graphics::renderTexture()
{
	static float times = 0;
	times++;
	float progress = sin((float)times / 50);

	float ratio = width / (float) height;


	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, ratio, 0.01, 10.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.5, 0.5, 0, 0.5, 0.5, 0.5, 0.0, 1.0, 0.0);

	
	// Emit a quad
	glBindTexture(GL_TEXTURE_2D, texture);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex3f(0, 0, 0.5);
		glTexCoord2f(0, 1);
		glVertex3f(0, 1, 0.5);
		glTexCoord2f(1, 1);
		glVertex3f(1, 1, 0.5);
		glTexCoord2f(1, 0);
		glVertex3f(1, 0, 0.5);
	glEnd();

	glfwSwapBuffers(window);
}
