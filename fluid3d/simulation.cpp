#include "simulation.h"
#include "main.h"
#include "File.h"

/*
 * Generic initialisation
 */
void Simulation::initialize(int volumeSide)
{
	//Set parameters
	N = volumeSide;
	voxels = (N+2)*(N+2)*(N+2); 
	size = voxels * sizeof(float);

	solverSteps = 20;
	dt = 0.1; //timestep
	visc = 0.001; //viscosity (velocity)
	diff = 0.0005; //dampening (density)

	allocateBuffers();

	resampleKernel = new cl::Kernel(*opencl.program, "resample", &opencl.err);


	/* Future work:
	clCreateFromGLTexture(context(), CL_MEM_WRITE_ONLY, GL_TEXTURE_3D, 0, texture, &err);
	checkErr(err, "Creating OpenCL 3D texture");
	*/
}

/*
 * Parallel initialisation (many kernels)
 */
void ParallelSimulation::initialize(int volumeSide)
{
	Simulation::initialize(volumeSide);

	diffuseKernel = new cl::Kernel(*opencl.program, "diffuse", &opencl.err);
	//diffuseKernelImage3D = new cl::Kernel(*opencl.program, "diffuse_", &opencl.err);

	advectKernel = new cl::Kernel(*opencl.program, "advect", &opencl.err);

	setBoundKernel = new cl::Kernel(*opencl.program, "setBound", &opencl.err);

	addSourceKernel = new cl::Kernel(*opencl.program, "addSource", &opencl.err);

	projectKernel1 = new cl::Kernel(*opencl.program, "project1", &opencl.err);
	projectKernel2 = new cl::Kernel(*opencl.program, "project2", &opencl.err);
	projectKernel3 = new cl::Kernel(*opencl.program, "project3", &opencl.err);


	setKernelArguments();
}

/*
 * Sequential initialisation (1 kernel)
 */
void SequentialSimulation::initialize(int volumeSide)
{
	Simulation::initialize(volumeSide);

	fluidKernel = new cl::Kernel(*opencl.program, "fluid");
	opencl.checkErr("Kernel::Kernel() (fluid)");
	setKernelArguments();
}

/*
 * Non-changing arguments
 */
void ParallelSimulation::setKernelArguments()
{
	//Diffuse kernel
	diffuseKernel->setArg(0, N);
	//diffuseKernelImage3D->setArg(0, N);

	//Advect kernel
	advectKernel->setArg(0, N);
	advectKernel->setArg(1, 0);
	advectKernel->setArg(7, (float)dt);

	//Set Bound kernel
	setBoundKernel->setArg(0, N);

	//Add source kernel
	addSourceKernel->setArg(0, dt);

	//Project kernel
	projectKernel1->setArg(0, N);
	projectKernel2->setArg(0, N);
	projectKernel3->setArg(0, N);
}

void SequentialSimulation::setKernelArguments()
{
	fluidKernel->setArg(0, N);
	fluidKernel->setArg(1, visc);
	fluidKernel->setArg(2, diff);
	fluidKernel->setArg(3, dt);
	fluidKernel->setArg(4, *buf_u);
	fluidKernel->setArg(5, *buf_v);
	fluidKernel->setArg(6, *buf_w);
	fluidKernel->setArg(7, *buf_u_prev);
	fluidKernel->setArg(8, *buf_v_prev);
	fluidKernel->setArg(9, *buf_w_prev);
	fluidKernel->setArg(10, *buf_dens);
	fluidKernel->setArg(11, *buf_dens_prev);
}

ParallelSimulation::~ParallelSimulation()
{
	delete diffuseKernel;
	delete advectKernel;
	delete setBoundKernel;
	delete addSourceKernel;
	delete projectKernel1; delete projectKernel2; delete projectKernel3;
}

SequentialSimulation::~SequentialSimulation()
{
	delete fluidKernel;
}

/*
 * Prepares all memory objects and OpenCL buffers wrappers for them
 * Needs CL context to be created!
 *
 * all - allocate all buffers, as opposed to just the temporaries
 * 	 useful in reallocating all memory objects by the resizer
 */
void Simulation::allocateBuffers(bool all)
{
	/* Image3D
	cl_image_format imageFormat;
	imageFormat.image_channel_order = CL_R;
	imageFormat.image_channel_data_type = CL_FLOAT;
	*/

	if(all)
	{
		u = new float[voxels];
		v = new float[voxels];
		w = new float[voxels];
		dens = new float[voxels];

		memset(u, 0, size);
		memset(v, 0, size);
		memset(w, 0, size);
		memset(dens, 0, size);

		buf_u = new cl::Buffer(*opencl.context, CL_MEM_USE_HOST_PTR, size, u, &opencl.err);
		buf_v = new cl::Buffer(*opencl.context, CL_MEM_USE_HOST_PTR, size, v, &opencl.err);
		buf_w = new cl::Buffer(*opencl.context, CL_MEM_USE_HOST_PTR, size, w, &opencl.err);
		buf_dens = new cl::Buffer(*opencl.context, CL_MEM_USE_HOST_PTR, size, dens, &opencl.err);

		//Image3D: Creating the image object for dens
		//image_dens = clCreateImage3D((*opencl.context)(), CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, &imageFormat, N+2, N+2, N+2, 0, 0, dens, &opencl.err);
	}

	//Temporary working buffers (non-rendered)
	u_prev = new float[voxels];
	v_prev = new float[voxels];
	w_prev = new float[voxels];
	dens_prev = new float[voxels];

	memset(u_prev, 0, size);
	memset(v_prev, 0, size);
	memset(w_prev, 0, size);
	memset(dens_prev, 0, size);

	buf_u_prev = new cl::Buffer(*opencl.context, CL_MEM_USE_HOST_PTR, size, u_prev, &opencl.err);
	buf_v_prev = new cl::Buffer(*opencl.context, CL_MEM_USE_HOST_PTR, size, v_prev, &opencl.err);
	buf_w_prev = new cl::Buffer(*opencl.context, CL_MEM_USE_HOST_PTR, size, w_prev, &opencl.err);
	buf_dens_prev = new cl::Buffer(*opencl.context, CL_MEM_USE_HOST_PTR, size, dens_prev, &opencl.err);

	//Image3D objects
	/*
	image_dens_prev = clCreateImage3D((*opencl.context)(), CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, &imageFormat, N+2, N+2, N+2, 0, 0, dens_prev, &opencl.err);
	image_write_dens_prev = clCreateImage3D((*opencl.context)(), CL_MEM_WRITE_ONLY|CL_MEM_USE_HOST_PTR, &imageFormat, N+2, N+2, N+2, 0, 0, dens_prev, &opencl.err);
	*/
}

void Simulation::deallocateBuffers()
{
	delete buf_u;
	delete buf_v;
	delete buf_w;
	delete buf_u_prev;
	delete buf_v_prev;
	delete buf_w_prev;
	delete buf_dens;
	delete buf_dens_prev;

	//Image3D: release
	//clReleaseMemObject(image_dens); 

	delete[] u;
	delete[] v;
	delete[] w;
	delete[] u_prev;
	delete[] v_prev;
	delete[] w_prev;
	delete[] dens;
	delete[] dens_prev;
}

//Resizes the simulation volume to newN
void Simulation::resize(int newN)
{
	int N0 = N;
	N = newN;
	voxels = (N+2)*(N+2)*(N+2);
	size = voxels * sizeof(float);

	//Only dens, u, v, w need to be reallocated
	float* u_new = new float[voxels];
	float* v_new = new float[voxels];
	float* w_new = new float[voxels];
	float* dens_new = new float[voxels];

	memset(u_new, 0, size);
	memset(v_new, 0, size);
	memset(w_new, 0, size);
	memset(dens_new, 0, size);

	//New buffer objects
	cl::Buffer* buf_u_new = new cl::Buffer(*opencl.context, CL_MEM_USE_HOST_PTR, size, u_new, &opencl.err);
	cl::Buffer* buf_v_new = new cl::Buffer(*opencl.context, CL_MEM_USE_HOST_PTR, size, v_new, &opencl.err);
	cl::Buffer* buf_w_new = new cl::Buffer(*opencl.context, CL_MEM_USE_HOST_PTR, size, w_new, &opencl.err);
	cl::Buffer* buf_dens_new = new cl::Buffer(*opencl.context, CL_MEM_USE_HOST_PTR, size, dens_new, &opencl.err);

	/* Image3D:
	cl_image_format imageFormat;
	imageFormat.image_channel_order = CL_R;
	imageFormat.image_channel_data_type = CL_FLOAT;
	cl_mem image_dens_new = clCreateImage3D((*opencl.context)(), CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, &imageFormat, N+2, N+2, N+2, 0, 0, dens_new, &opencl.err);
	*/

	//Resample whole volume (including bounds)
	resampleKernel->setArg(0, N + 2);
	resampleKernel->setArg(1, N0 + 2);
	resampleKernel->setArg(2, *buf_dens_new);
	resampleKernel->setArg(3, *buf_dens);
	resampleKernel->setArg(4, *buf_u_new);
	resampleKernel->setArg(5, *buf_u);
	resampleKernel->setArg(6, *buf_v_new);
	resampleKernel->setArg(7, *buf_v);
	resampleKernel->setArg(8, *buf_w_new);
	resampleKernel->setArg(9, *buf_w);
	opencl.enqueue(*resampleKernel, cl::NullRange, cl::NDRange(min(N,N0) + 2, min(N,N0) + 2, min(N,N0) + 2), cl::NullRange);
	opencl.wait();

	deallocateBuffers(); //delete all current buffers
	allocateBuffers(false); //allocate only temporary buffers (_prev)

	//Assign the non-_prev buffers to resized data
	u = u_new;
	v = v_new;
	w = w_new;
	dens = dens_new;
	buf_u = buf_u_new;
	buf_v = buf_v_new;
	buf_w = buf_w_new;
	buf_dens = buf_dens_new;
	//image_dens = image_dens_new; //Image3D
	
	setKernelArguments(); //N has changed, arguments need to be re-set
}

/*
 * Sequential step function - simple, 1 kernel
 */
void SequentialSimulation::step()
{
	opencl.enqueue( *fluidKernel, cl::NullRange, cl::NDRange(1), cl::NullRange);
	opencl.checkErr("ComamndQueue::enqueueNDRangeKernel() (simulate)");
	opencl.wait();

	// Clear the garbage in dens_prev (it was used as a temp buffer)
	memset(dens_prev, 0, size);
}

/*
 * One parallel simulation step - enacts the simulation pipeline using OpenCL
 *
 * All the action takes place here.
 *
 * All kernel calls are on an extra indentation under the comments showing their 
 * source lines in the C version. Hope this makes it easier to read.
 */
void ParallelSimulation::step()
{
	// add_source ( N, u, u0, dt ); add_source ( N, v, v0, dt ); add_source ( N, w, w0, dt );
		addSourceKernel->setArg(1, *buf_u);
		addSourceKernel->setArg(2, *buf_u_prev);
		opencl.enqueue(*addSourceKernel, cl::NullRange, cl::NDRange(voxels / 4), cl::NullRange);
		opencl.wait();

		addSourceKernel->setArg(1, *buf_v);
		addSourceKernel->setArg(2, *buf_v_prev);
		opencl.enqueue(*addSourceKernel, cl::NullRange, cl::NDRange(voxels / 4), cl::NullRange);
		opencl.wait();

		addSourceKernel->setArg(1, *buf_w);
		addSourceKernel->setArg(2, *buf_w_prev);
		opencl.enqueue(*addSourceKernel, cl::NullRange, cl::NDRange(voxels / 4), cl::NullRange);
		opencl.wait();


	//SWAP ( u0, u ); diffuse ( N, 1, u, u0, visc, dt);
	//SWAP ( v0, v ); diffuse ( N, 2, v, v0, visc, dt);
	//SWAP ( w0, w ); diffuse ( N, 3, w, w0, visc, dt);
		diffuseKernel->setArg(1, (float)dt*visc*N*N);
		diffuseKernel->setArg(2, *buf_u_prev);
		diffuseKernel->setArg(3, *buf_u);
		for(int i = 0; i < solverSteps; i++)
		{
			opencl.enqueue(*diffuseKernel, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
			opencl.wait();

			setBoundKernel->setArg(1, 1);
			setBoundKernel->setArg(2, *buf_u_prev);
			opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
			opencl.wait();
		}

		diffuseKernel->setArg(2, *buf_v_prev);
		diffuseKernel->setArg(3, *buf_v);
		for(int i = 0; i < solverSteps; i++)
		{
			opencl.enqueue(*diffuseKernel, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
			opencl.wait();

			setBoundKernel->setArg(1, 2);
			setBoundKernel->setArg(2, *buf_v_prev);
			opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
			opencl.wait();
		}

		diffuseKernel->setArg(2, *buf_w_prev);
		diffuseKernel->setArg(3, *buf_w);
		for(int i = 0; i < solverSteps; i++)
		{
			opencl.enqueue(*diffuseKernel, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
			opencl.wait();

			setBoundKernel->setArg(1, 3);
			setBoundKernel->setArg(2, *buf_w_prev);
			opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
			opencl.wait();
		}

	//project ( N, u, v, w, u0, v0 ); (part 1)
		projectKernel1->setArg(1, *buf_u_prev); projectKernel2->setArg(1, *buf_u_prev); projectKernel3->setArg(1, *buf_u_prev);
		projectKernel1->setArg(2, *buf_v_prev); projectKernel2->setArg(2, *buf_v_prev); projectKernel3->setArg(2, *buf_v_prev);
		projectKernel1->setArg(3, *buf_w_prev); projectKernel2->setArg(3, *buf_w_prev); projectKernel3->setArg(3, *buf_w_prev);
		projectKernel1->setArg(4, *buf_u); projectKernel2->setArg(4, *buf_u); projectKernel3->setArg(4, *buf_u);
		projectKernel1->setArg(5, *buf_v); projectKernel2->setArg(5, *buf_v); projectKernel3->setArg(5, *buf_v);
		opencl.enqueue(*projectKernel1, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
		opencl.wait();

		//setBoundSeq (N, 0, div ); setBoundSeq (N, 0, p );
		setBoundKernel->setArg(1, 0);
		setBoundKernel->setArg(2, *buf_u);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
		setBoundKernel->setArg(2, *buf_v);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);

	//project ( N, u, v, w, u0, v0 ); (part 2)
		for(int i = 0; i < solverSteps; i++)
		{
			opencl.enqueue(*projectKernel2, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
			opencl.wait();

			setBoundKernel->setArg(1, 0);
			setBoundKernel->setArg(2, *buf_u);
			opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
			opencl.wait();
		}
		
	//project ( N, u, v, w, u0, v0 ); (part 3)
		opencl.enqueue(*projectKernel3, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
		opencl.wait();
	
		//setBoundSeq (N, 1, u ); setBoundSeq (N, 2, v ); setBoundSeq (N, 3, w );
		setBoundKernel->setArg(1, 1);
		setBoundKernel->setArg(2, *buf_u_prev);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
		setBoundKernel->setArg(1, 2);
		setBoundKernel->setArg(2, *buf_v_prev);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
		setBoundKernel->setArg(1, 3);
		setBoundKernel->setArg(2, *buf_w_prev);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
		opencl.wait();

	//advect ( N, 1, u, u0, u0, v0, w0, dt );
	//advect ( N, 2, v, v0, u0, v0, w0, dt );
	//advect ( N, 3, w, w0, u0, v0, w0, dt );
		advectKernel->setArg(1, 1);
		advectKernel->setArg(2, *buf_u);
		advectKernel->setArg(3, *buf_u_prev);
		advectKernel->setArg(4, *buf_u_prev);
		advectKernel->setArg(5, *buf_v_prev);
		advectKernel->setArg(6, *buf_w_prev);
		opencl.enqueue(*advectKernel, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
		opencl.wait();
		setBoundKernel->setArg(1, 1);
		setBoundKernel->setArg(2, *buf_u);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
		opencl.wait();

		advectKernel->setArg(1, 2);
		advectKernel->setArg(2, *buf_v);
		advectKernel->setArg(3, *buf_v_prev);
		opencl.enqueue(*advectKernel, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
		opencl.wait();
		setBoundKernel->setArg(1, 2);
		setBoundKernel->setArg(2, *buf_v);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
		opencl.wait();

		advectKernel->setArg(1, 3);
		advectKernel->setArg(2, *buf_w);
		advectKernel->setArg(3, *buf_w_prev);
		opencl.enqueue(*advectKernel, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
		opencl.wait();
		setBoundKernel->setArg(1, 3);
		setBoundKernel->setArg(2, *buf_w);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
		opencl.wait();

	//project ( N, u, v, w, u0, v0 ); (part 1)
		projectKernel1->setArg(1, *buf_u); projectKernel2->setArg(1, *buf_u); projectKernel3->setArg(1, *buf_u);
		projectKernel1->setArg(2, *buf_v); projectKernel2->setArg(2, *buf_v); projectKernel3->setArg(2, *buf_v);
		projectKernel1->setArg(3, *buf_w); projectKernel2->setArg(3, *buf_w); projectKernel3->setArg(3, *buf_w);
		projectKernel1->setArg(4, *buf_u_prev); projectKernel2->setArg(4, *buf_u_prev); projectKernel3->setArg(4, *buf_u_prev);
		projectKernel1->setArg(5, *buf_v_prev); projectKernel2->setArg(5, *buf_v_prev); projectKernel3->setArg(5, *buf_v_prev);
		opencl.enqueue(*projectKernel1, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
		opencl.wait();

		//setBoundSeq (N, 0, div ); setBoundSeq (N, 0, p );
		setBoundKernel->setArg(1, 0);
		setBoundKernel->setArg(2, *buf_u_prev);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
		setBoundKernel->setArg(2, *buf_v_prev);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);

	//project ( N, u, v, w, u0, v0 ); (part 2)
		for(int i = 0; i < solverSteps; i++)
		{
			opencl.enqueue(*projectKernel2, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
			opencl.wait();

			setBoundKernel->setArg(1, 0);
			setBoundKernel->setArg(2, *buf_u_prev);
			opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
			opencl.wait();
		}

	//project ( N, u, v, w, u0, v0 ); (part 3)
		opencl.enqueue(*projectKernel3, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
		opencl.wait();

		//setBoundSeq (N, 1, u ); setBoundSeq (N, 2, v ); setBoundSeq (N, 3, w );
		setBoundKernel->setArg(1, 1);
		setBoundKernel->setArg(2, *buf_u);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
		setBoundKernel->setArg(1, 2);
		setBoundKernel->setArg(2, *buf_v);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
		setBoundKernel->setArg(1, 3);
		setBoundKernel->setArg(2, *buf_w);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
		opencl.wait();

//dens_step:
	//add_source ( N, x, x0, dt );
		addSourceKernel->setArg(1, *buf_dens);
		addSourceKernel->setArg(2, *buf_dens_prev);
		opencl.enqueue(*addSourceKernel, cl::NullRange, cl::NDRange(voxels / 4), cl::NullRange);
		opencl.wait();

	//Image3D:
	//SWAP ( x0,x ); diffuse ( N, 0, x, x0, diff, dt );
	/*
		diffuseKernelImage3D->setArg(1, (float)dt*diff*N*N);
		diffuseKernelImage3D->setArg(2, image_dens_prev);
		diffuseKernelImage3D->setArg(3, image_dens);
		diffuseKernelImage3D->setArg(4, image_write_dens_prev);
		for(int i = 0; i < solverSteps; i++)
		{
			opencl.enqueue(*diffuseKernelImage3D, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
			opencl.wait();

			//set_bnd ( N, b = 0, x );
			setBoundKernel->setArg(1, 0);
			setBoundKernel->setArg(2, *buf_dens_prev);
			opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
			opencl.wait();
		}
		opencl.wait();
	*/

		diffuseKernel->setArg(1, (float)dt*diff*N*N);
		diffuseKernel->setArg(2, *buf_dens_prev);
		diffuseKernel->setArg(3, *buf_dens);
		for(int i = 0; i < solverSteps; i++)
		{
			opencl.enqueue(*diffuseKernel, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);

			//set_bnd ( N, b = 0, x );
			setBoundKernel->setArg(1, 0);
			setBoundKernel->setArg(2, *buf_dens_prev);
			opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
			opencl.wait();
		}
		opencl.wait();
	

	// SWAP ( x0,x ); advect ( N, 0, x, x0, u, v, w, dt );
		advectKernel->setArg(0, N);
		advectKernel->setArg(1, 0);
		advectKernel->setArg(2, *buf_dens);
		advectKernel->setArg(3, *buf_dens_prev);
		advectKernel->setArg(4, *buf_u);
		advectKernel->setArg(5, *buf_v);
		advectKernel->setArg(6, *buf_w);
		opencl.enqueue(*advectKernel, cl::NullRange, cl::NDRange(N, N, N), cl::NullRange);
		opencl.wait();

		// set_bnd ( N, b, d );
		setBoundKernel->setArg(1, 0);
		setBoundKernel->setArg(2, *buf_dens);
		opencl.enqueue(*setBoundKernel, cl::NullRange, cl::NDRange(N + 1, N + 1), cl::NullRange);
		opencl.wait();

	// Clear the garbage in dens_prev (it was used as a temp buffer)
	memset(dens_prev, 0, size);
}

//3d -> 1d indexer into the volume, used for setting data and debugging
inline int Simulation::ix(int i, int j, int l)
{
	return ((i)+(N+2)*(j)+(N+2)*(N+2)*l);
}

//Clears everything
void Simulation::reset()
{
	memset(u, 0, size);
	memset(v, 0, size);
	memset(w, 0, size);
	memset(dens, 0, size);
	clearEffects();
}

void Simulation::addFluid()
{
	cout<<"adding fluid"<<endl;
	dens_prev[ix(6, 6, 6)] = 10.0;
}

void Simulation::addForce()
{
	u_prev[ix(2, 2, 2)] = 1000.0;
	v_prev[ix(2, 2, 2)] = 1000.0;
	w_prev[ix(2, 2, 2)] = 1000.0;
}

//Clears just the velocity
void Simulation::clearEffects()
{
	cout<<"clearing effects"<<endl;
	memset(u_prev, 0, size);
	memset(v_prev, 0, size);
	memset(w_prev, 0, size);
	memset(dens_prev, 0, size);
}

//Not used for the time being
void Simulation::debug()
{
	float sum = 0, sump = 0;
	for(int i = 0; i < N; i++)
		for(int j = 0; j < N; j++)
			for(int k = 0; k < N; k++)
			{
				sum += dens[ix(i, j, k)];
				sump += dens_prev[ix(i, j, k)];
			}
	cout<<"sum/prev: "<<sum<<" "<<sump<<endl;
}
