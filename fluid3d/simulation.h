/*
 * The main simulation class - conducts the simulation, keeps
 * state and uses OpenCL to perform the calculations
 *
 * Is friends with the Tuner class, which changes parameters.
 */
#pragma once 
#include <CL/cl.hpp>


class Simulation
{
public:
	Simulation() {};
	virtual ~Simulation()
	{
		delete resampleKernel;
		deallocateBuffers();
	}

	friend class Tuner;

	// General control
	virtual void initialize(int);
	virtual void setKernelArguments() = 0;
	virtual void step() = 0;
	void allocateBuffers(bool = true);
	void deallocateBuffers();
	void debug();

	// Getters
	cl::Buffer* getOutputVolume() { return buf_dens; }
	int getN() {return N;}

	// Volume modification
	inline int ix(int, int, int);
	void addFluid(); 
	void addForce();
	void clearEffects();
	void reset();
	void resize(int);
	
protected:
	// Simulation buffers
	float *u,
	      *v,
	      *w,
	      *u_prev,
	      *v_prev,
	      *w_prev,
	      *dens,
	      *dens_prev;
	cl::Buffer *buf_u, *buf_v, *buf_w, *buf_u_prev, *buf_v_prev, *buf_w_prev, *buf_dens, *buf_dens_prev;
	cl::Kernel *resampleKernel;

	//Image3D: Image objects for the modified density kernel
	//cl_mem image_dens, image_dens_prev, image_write_dens_prev;

	// Simulation kernels
	int N ;
	int voxels;
	int size;
	float dt; //timestep
	float visc;
	float diff; //dampening
	int solverSteps;

};

class ParallelSimulation : public Simulation
{
public:
	~ParallelSimulation();
	void initialize(int);
	void setKernelArguments();
	void step();
private:
	cl::Kernel *diffuseKernel, *advectKernel,
		*setBoundKernel, *addSourceKernel,
		*projectKernel1, *projectKernel2, *projectKernel3;
	//*diffuseKernelImage3D;
};

class SequentialSimulation : public Simulation
{
public:
	~SequentialSimulation();
	void initialize(int);
	void setKernelArguments();
	void step();
private:
	cl::Kernel *fluidKernel;
};
