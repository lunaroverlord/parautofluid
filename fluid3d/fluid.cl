/*
* 3D version of the fluid solver on OpenCL - the one being used.
* Main simulation code, contains kernels for all the stages.
* 
* Variables:
* N - the simulation resolution
* dt - the timestep
* a - an intermediate value derived from the viscosity parameters
* u, v, w - velocity field
* x - scalar field
*/
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
#pragma OPENCL EXTENSION cl_khr_3d_image_writes : enable

//Index and swap
#define IX(i,j,l) ((i)+(N+2)*(j)+(N+2)*(N+2)*(l))
#define SWAP(x0,x) {float *tmp=x0;x0=x;x=tmp;}

//addSource: adds a field (s) to another (x)
__kernel void addSource (float dt, __global float * x, __global float * s)
{
	int i = get_global_id(0) * 4;

	//Use coalesced memory accesses
	x[i] += dt*s[i];
	x[i+1] += dt*s[i+1];
	x[i+2] += dt*s[i+2];
	x[i+3] += dt*s[i+3];
}

//setBound: zero out velocity and density on the walls that would otherwies make the fluid leave the container
//b: 
//1 - flip sign for y-aligned sides
//2 - flip sign for x-aligned sides
//3 - flip sign for z-aligned sides
__kernel void setBound(int N, int b, __global float * x) 
{ 
	int i = get_global_id(0);
	int j = get_global_id(1);

	//Faces (6 in total)
	if(i >= 1 && j >= 1)
	{
		//Front, back faces
		x[IX(i, j, 0)] = b==3 ? -x[IX(i,j,1)] : x[IX(i,j,1)]; 
		x[IX(i, j, N+1)] = b==3 ? -x[IX(i,j,N)] : x[IX(i,j,N)]; 
		//Top, bottom faces
		x[IX(i, N+1, j)] = b==2 ? -x[IX(i,N,j)] : x[IX(i,N,j)]; 
		x[IX(i, 0, j)] = b==2 ? -x[IX(i,1,j)] : x[IX(i,1,j)]; 
		//Left, right faces
		x[IX(0, i, j)] = b==1 ? -x[IX(1,i,j)] : x[IX(1,i,j)]; 
		x[IX(N+1,i, j)] = b==1 ? -x[IX(N,i,j)] : x[IX(N,i,j)]; 
	}
	//Edges (12 in total)
	else if(i >= 1 && j == 0)
	{
		//Front face edges
		x[IX(0  ,i, 0)] = x[IX(1,i,1)]; 
		x[IX(N+1,i, 0)] = x[IX(N,i,1)]; 
		x[IX(i,0  , 0)] = x[IX(i,1,1)]; 		
		x[IX(i,N+1, 0)] = x[IX(i,N,1)]; 		

		//Back face edges
		x[IX(0  ,i, N+1)] = x[IX(1,i,N)]; 
		x[IX(N+1,i, N+1)] = x[IX(N,i,N)]; 
		x[IX(i,0  , N+1)] = x[IX(i,1,N)]; 		
		x[IX(i,N+1, N+1)] = x[IX(i,N,N)]; 		
		
		//Bottom edges of the z-faces
		x[IX(0  ,0, i)] =  x[IX(1,1,i)]; 
		x[IX(N+1,0, i)] =  x[IX(N,1,i)]; 
		//Top edges of the z-faces
		x[IX(0,  N+1,i)] = x[IX(1,N,i)]; 		
		x[IX(N+1,N+1,i)] = x[IX(N,N,i)]; 		
	} 
	//Corners
	else if(i == 0 && j == 0)
	{
		x[IX(0  ,0  , 0  )] = x[IX(1, 1, 1)];
		x[IX(0  ,N+1, 0  )] = x[IX(1, N, 1)];
		x[IX(N+1,0  , 0  )] = x[IX(N, 1, 1)];
		x[IX(N+1,N+1, 0  )] = x[IX(N, N, 1)];
		x[IX(0  ,0  , N+1)] = x[IX(1, 1, N)];
		x[IX(0  ,N+1, N+1)] = x[IX(1, N, N)];
		x[IX(N+1,0  , N+1)] = x[IX(N, 1, N)];
		x[IX(N+1,N+1, N+1)] = x[IX(N, N, N)];
	}
} 

//Project phase 1
__kernel void project1( int N, __global float * u, __global float * v, __global float * w, __global float * p, __global float * div )
{
	int i = get_global_id(0) + 1;
	int j = get_global_id(1) + 1;
	int l = get_global_id(2) + 1;
	float h = 1.0/N;

	div[IX(i,j,l)] = -0.5*h*(
			u[IX(i+1,j,l)]-u[IX(i-1,j,l)]+
			v[IX(i,j+1,l)]-v[IX(i,j-1,l)]+
			w[IX(i,j,l+1)]-w[IX(i,j,l-1)]);
	p[IX(i,j,l)] = 0;
}

//Project phase 2 (iterative)
__kernel void project2( int N, __global float * u, __global float * v, __global float * w, __global float * p, __global float * div )
{
	int i = get_global_id(0) + 1;
	int j = get_global_id(1) + 1;
	int l = get_global_id(2) + 1;

	p[IX(i,j,l)] = (div[IX(i,j,l)]
		+p[IX(i-1,j,l)]+p[IX(i+1,j,l)]
		+p[IX(i,j-1,l)]+p[IX(i,j+1,l)]
		+p[IX(i,j,l-1)]+p[IX(i,j,l+1)])/6;
}

//Project phase 3
__kernel void project3( int N, __global float * u, __global float * v, __global float * w, __global float * p, __global float * div )
{
	int i = get_global_id(0) + 1;
	int j = get_global_id(1) + 1;
	int l = get_global_id(2) + 1;

	float h = 1.0/N;

	u[IX(i,j,l)] -= 0.5*(p[IX(i+1,j,l)]-p[IX(i-1,j,l)])/h;
	v[IX(i,j,l)] -= 0.5*(p[IX(i,j+1,l)]-p[IX(i,j-1,l)])/h;
	w[IX(i,j,l)] -= 0.5*(p[IX(i,j,l+1)]-p[IX(i,j,l-1)])/h;
}

//Diffusion part (also iterative)
__kernel void diffuse (int N, float a, __global float * x, __global float * x0)
{
	int i = get_global_id(0) + 1;
	int j = get_global_id(1) + 1;
	int l = get_global_id(2) + 1;

	float t1 = x[IX(i-1,j,l)]+x[IX(i+1,j,l)];
	float t2 = x[IX(i,j-1,l)]+x[IX(i,j+1,l)];
	float t3 = x[IX(i,j,l-1)]+x[IX(i,j,l+1)];

	x[IX(i,j,l)] = (x0[IX(i,j,l)] +	a*(t1 + t2 + t3)) / (1+6*a);;
}

//The twice slower image3d version (not used)
/*
__kernel void diffuse_image3d (int N, float a, __read_only image3d_t x, __read_only image3d_t x0, __write_only image3d_t x_out)
{
	int i = get_global_id(0) + 1;
	int j = get_global_id(1) + 1;
	int l = get_global_id(2) + 1;

	//printf("%v4f", t1_1);
	float t1 = read_imagef(x, (int4)(i-1,j,l,0)).x + read_imagef(x, (int4)(i+1,j,l,0)).x;
	float t2 = read_imagef(x, (int4)(i,j-1,l,0)).x + read_imagef(x, (int4)(i,j+1,l,0)).x;
	float t3 = read_imagef(x, (int4)(i,j,l-1,0)).x + read_imagef(x, (int4)(i,j,l+1,0)).x;

	float x0source = read_imagef(x0, (int4)(i,j,l,0)).x; 
	write_imagef(x_out, (int4)(i,j,l,0), (x0source + a*(t1 + t2 + t3)) / (1+6*a));
	//int i, j, l, k;
	//float a=dt*diff*N*N;
}
*/

//Advection
__kernel void advect ( int N, int b,
	__global float * d, __global float * d0, 
	__global float * u, __global float * v, __global float * w, float dt )
{
	int i = get_global_id(0) + 1;
	int j = get_global_id(1) + 1;
	int l = get_global_id(2) + 1;
	int i0, j0, l0, i1, j1, l1;
	float x, y, z, s0, t0, r0, s1, t1, r1, dt0;
	dt0 = dt*N;

	//Trace velocity back to corrdinates (x, y, z)
	//(current cell) - (velocity vector) = (source point)
	x = (float)i-dt0*u[IX(i,j,l)];
	y = (float)j-dt0*v[IX(i,j,l)];
	z = (float)l-dt0*w[IX(i,j,l)];

	//1. Prevent going out of bounds
	//2. Convert coordinates (xyz : float) to cells (ijl : int)
	//(i1,j1,l1) are (i0,j0,l1) + 1

	/* Non-linear:
	if (x<0.5) x=0.5; if (x>N+0.5) x=N+ 0.5; i0=(int)x; i1=i0+1;
	if (y<0.5) y=0.5; if (y>N+0.5) y=N+ 0.5; j0=(int)y; j1=j0+1;
	if (z<0.5) z=0.5; if (z>N+0.5) z=N+ 0.5; l0=(int)z; l1=l0+1;
	*/

	//Optimised for linearity:
	x = clamp(x, (float)0.5, (float)(N + 0.5)); i0=(int)x; i1=i0+1;
	y = clamp(y, (float)0.5, (float)(N + 0.5)); j0=(int)y; j1=j0+1;
	z = clamp(z, (float)0.5, (float)(N + 0.5)); l0=(int)z; l1=l0+1;


	//str = the error of (i,j,l) from (xyz) from the rounding
	//used for interpolation, both < 1
	s1 = x-i0; s0 = 1-s1; 
	t1 = y-j0; t0 = 1-t1;
	r1 = z-l0; r0 = 1-r1;

	//Set current cell (ijl) to interpolated values
	//sampled from the source point's surrounding cells
	d[IX(i,j,l)] = 
		 s0*(r0*(t0*d0[IX(i0,j0,l0)]+t1*d0[IX(i0,j1,l0)])
		+r1*(t0*d0[IX(i0,j0,l1)]+t1*d0[IX(i0,j1,l1)]))
		+s1*(r0*(t0*d0[IX(i1,j0,l0)]+t1*d0[IX(i1,j1,l0)])
		+r1*(t0*d0[IX(i1,j0,l1)]+t1*d0[IX(i1,j1,l1)]));
}

//Special volume indexer - takes size into account
#define IXs(size,i,j,l) (int)((i)+(size)*(j)+(size)*(size)*(l))
//Floor/ceiling functions that don't overstep the bounds of the volume
float xfloor(float value)
{
	return ceil(value - 1);
}
float xceil(float value)
{
	return floor(value + 1);
}

//Single voxel resampling from N0 to N
void resampleVoxel(int N, int N0, __global float* source, __global float* destination)
{
	//ijl indexes destination
	size_t i = get_global_id(0);
	size_t j = get_global_id(1);
	size_t l = get_global_id(2);

	//N0 > N
	float ratio = (float)N0 / N;	


	//Find which points in source to sample from:
	//Base of source cube
	float ib0 = (float)i * ratio;
	float jb0 = (float)j * ratio;
	float lb0 = (float)l * ratio;
	//Furthest corner of source cube
	float ib1 = (float)(i + 1) * ratio;
	float jb1 = (float)(j + 1) * ratio;
	float lb1 = (float)(l + 1) * ratio;

	//Fix possible floating point error caused by the addition above
	ib1 = round(ib1 * 10000) / 10000;
	jb1 = round(jb1 * 10000) / 10000;
	lb1 = round(lb1 * 10000) / 10000;

	//Accumulate volume density from each corner of the cube
	float deposit = 0;
	deposit += source[IXs(N0, (int)      (ib0), (int)      (jb0), (int)      (lb0))] * (xceil(ib0) - ib0 ) * (xceil(jb0) - jb0 ) * (xceil(lb0) - lb0 );
	deposit += source[IXs(N0, (int)      (ib0), (int)      (jb0), (int)xfloor(lb1))] * (xceil(ib0) - ib0 ) * (xceil(jb0) - jb0 ) * (lb1 - xfloor(lb1));
	deposit += source[IXs(N0, (int)      (ib0), (int)xfloor(jb1), (int)      (lb0))] * (xceil(ib0) - ib0 ) * (jb1 - xfloor(jb1)) * (xceil(lb0) - lb0 );
	deposit += source[IXs(N0, (int)      (ib0), (int)xfloor(jb1), (int)xfloor(lb1))] * (xceil(ib0) - ib0 ) * (jb1 - xfloor(jb1)) * (lb1 - xfloor(lb1));
	deposit += source[IXs(N0, (int)xfloor(ib1), (int)      (jb0), (int)      (lb0))] * (ib1 - xfloor(ib1)) * (xceil(jb0) - jb0 ) * (xceil(lb0) - lb0 );
	deposit += source[IXs(N0, (int)xfloor(ib1), (int)      (jb0), (int)xfloor(lb1))] * (ib1 - xfloor(ib1)) * (xceil(jb0) - jb0 ) * (lb1 - xfloor(lb1));
	deposit += source[IXs(N0, (int)xfloor(ib1), (int)xfloor(jb1), (int)      (lb0))] * (ib1 - xfloor(ib1)) * (jb1 - xfloor(jb1)) * (xceil(lb0) - lb0 );
	deposit += source[IXs(N0, (int)xfloor(ib1), (int)xfloor(jb1), (int)xfloor(lb1))] * (ib1 - xfloor(ib1)) * (jb1 - xfloor(jb1)) * (lb1 - xfloor(lb1));
	
	destination[IXs(N,i,j,l)] += deposit;
}

//This is called from the program, it resamples all fields that constitute the volume
__kernel void resample(int N, int N0, __global float* dens, __global float* dens0, 
			__global float* u, __global float* u0, 
			__global float* v, __global float* v0, 
			__global float* w, __global float* w0)
{
	/*
	if(get_global_id(0) + get_global_id(1) + get_global_id(2) == 0)
		printf("resampling from %i to %i\n", N0, N);
		*/
	resampleVoxel(N, N0, dens0, dens);	
	resampleVoxel(N, N0, u0, u);	
	resampleVoxel(N, N0, v0, v);	
	resampleVoxel(N, N0, w0, w);	
}
