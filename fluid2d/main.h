#pragma once

#include <iostream>
#include <iomanip>
#include <cstring> //for memset
#include <unistd.h>

#include <GLFW/glfw3.h>
#include <GL/gl.h>

#define IX(i,j) ((i)+(N+2)*(j))
#define SWAP(x0,x) {float *tmp=x0;x0=x;x=tmp;}

using namespace std;

struct Vector
{
	int x, y;
};
