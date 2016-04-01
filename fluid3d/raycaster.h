/*
 * The class that calls the raycasting kernel
 * 256x256 hardcoded texture is produced, should be made more
 * flexible in the future
 */
#pragma once

#include <CL/cl.hpp>
#include "camera.h"

class RayCaster
{
public:
	RayCaster(){};
	~RayCaster()
	{
		delete buf_texture;
		delete[] textureData;
		delete rayCastKernel;
	}

	void initialize(int, cl::Buffer*);
	void shoot();
	unsigned char* getTexture();

	void setCamera(Camera&);
	void setN(int);
	void setVolume(cl::Buffer*);


private:
	//Kernel
	cl::Kernel *rayCastKernel;
	
	//Raycasting buffers
	cl::Buffer* buf_texture;
	cl::Buffer* volumeBuffer;
	int textureLength;
	unsigned char* textureData;
};
