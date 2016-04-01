#include "raycaster.h"
#include "main.h"

void RayCaster::initialize(int N, cl::Buffer* volume)
{
	//Output texture buffer allocation
	textureLength = 256*256*4;
	textureData = new unsigned char[textureLength];
	memset(textureData, 0, textureLength);
	buf_texture = new cl::Buffer(*opencl.context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, textureLength, textureData, &opencl.err);
	opencl.checkErr("Buffer::Buffer()");

	//OpenCL kernel initialisation
	rayCastKernel = new cl::Kernel(*opencl.program, "RayCaster", &opencl.err);
	opencl.checkErr("Kernel::Kernel() (raycast)");
	rayCastKernel->setArg(0, 256);
	rayCastKernel->setArg(1, 256);
	rayCastKernel->setArg(2, *buf_texture);
	rayCastKernel->setArg(3, 256);

	setN(N);
	setVolume(volume);
}

//Volume resolution
void RayCaster::setN(int N)
{
	rayCastKernel->setArg(9, N + 2);
}

//Volume data (pointer)
void RayCaster::setVolume(cl::Buffer* volume)
{
	rayCastKernel->setArg(8, *volume);
}

//Set position in 3D space
void RayCaster::setCamera(Camera& camera)
{
	rayCastKernel->setArg(4, sizeof(float[4]), camera.getPosition());
	rayCastKernel->setArg(5, sizeof(float[4]), camera.getForward());
	rayCastKernel->setArg(6, sizeof(float[4]), camera.getRight());
	rayCastKernel->setArg(7, sizeof(float[4]), camera.getUp());

}

//Produces the texture
void RayCaster::shoot()
{
	//Raycast
	opencl.err = opencl.queue->enqueueNDRangeKernel( *rayCastKernel, cl::NullRange, cl::NDRange(256, 256), cl::NullRange, NULL, &opencl.event);
	opencl.checkErr("ComamndQueue::enqueueNDRangeKernel() (render)");

	opencl.wait();

	opencl.err = opencl.queue->enqueueReadBuffer( *buf_texture, CL_TRUE, 0, textureLength, textureData);
	opencl.checkErr("ComamndQueue::enqueueReadBuffer()");
}

unsigned char* RayCaster::getTexture()
{
	return textureData;
}
