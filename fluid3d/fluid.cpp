//Plain C 3D version of the fluid solver, non-OpenCL (only for reference)
#include "fluid.h"

void add_source ( int N, float * x, float * s, float dt )
{
	int i, size=(N+2)*(N+2)*(N+2);
	for ( i=0 ; i<size ; i++ ) x[i] += dt*s[i];
}

//Zero out velocity on the walls that would otherwies make the fluid leave the container
//b: 
//1 - flip sign for y-aligned sides
//2 - flip sign for x-aligned sides
//3 - flip sign for z-aligned sides
void set_bnd ( int N, int b, float * x ) 
{ 
	int i,j;

	//Faces (6 in total)
	for ( i=1;  i<=N;  i++ ) { 
		for ( j=1;  j<=N;  j++ ) { 
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
	}

	//Sides (12 in total)
	for ( i=1;  i<=N;  i++ ) { 
		//Front face sides
		x[IX(0  ,i, 0)] = b==1 ? -x[IX(1,i,0)] : x[IX(1,i,0)]; 
		x[IX(N+1,i, 0)] = b==1 ? -x[IX(N,i,0)] : x[IX(N,i,0)]; 
		x[IX(i,0  , 0)] = b==2 ? -x[IX(i,1,0)] : x[IX(i,1,0)]; 		
		x[IX(i,N+1, 0)] = b==2 ? -x[IX(i,N,0)] : x[IX(i,N,0)]; 		

		//Back face sides
		x[IX(0  ,i, N+1)] = b==1 ? -x[IX(1,i,N+1)] : x[IX(1,i,N+1)]; 
		x[IX(N+1,i, N+1)] = b==1 ? -x[IX(N,i,N+1)] : x[IX(N,i,N+1)]; 
		x[IX(i,0  , N+1)] = b==2 ? -x[IX(i,1,N+1)] : x[IX(i,1,N+1)]; 		
		x[IX(i,N+1, N+1)] = b==2 ? -x[IX(i,N,N+1)] : x[IX(i,N,N+1)]; 		
		
		//Bottom sides of the z-faces
		x[IX(0  ,0, i)] = b==3 ? -x[IX(1,1,i)] : x[IX(1,1,i)]; 
		x[IX(N+1,0, i)] = b==3 ? -x[IX(N,0,i)] : x[IX(N,0,i)]; 
		//Top sides of the z-faces
		x[IX(0,N+1, i)] = b==3 ? -x[IX(1,N+1,i)] : x[IX(1,N+1,i)]; 		
		x[IX(N+1,N+1,i)] = b==3 ? -x[IX(N,N+1,i)] : x[IX(N,N+1,i)]; 		
	} 

	//Corners (8 in total)
	x[IX(0  ,0  , 0)] = 0.5*(x[IX(1,0  ,0)]+x[IX(0  ,1,0)]); 
	x[IX(0  ,N+1, 0)] = 0.5*(x[IX(1,N+1,0)]+x[IX(0  ,N,0)]); 
	x[IX(N+1,0  , 0)] = 0.5*(x[IX(N,0  ,0)]+x[IX(N+1,1,0)]); 
	x[IX(N+1,N+1, 0)] = 0.5*(x[IX(N,N+1,0)]+x[IX(N+1,N,0)]); 
	x[IX(0  ,0  , N+1)] = 0.5*(x[IX(1,0  ,N+1)]+x[IX(0  ,1,N+1)]); 
	x[IX(0  ,N+1, N+1)] = 0.5*(x[IX(1,N+1,N+1)]+x[IX(0  ,N,N+1)]); 
	x[IX(N+1,0  , N+1)] = 0.5*(x[IX(N,0  ,N+1)]+x[IX(N+1,1,N+1)]); 
	x[IX(N+1,N+1, N+1)] = 0.5*(x[IX(N,N+1,N+1)]+x[IX(N+1,N,N+1)]); 
} 

void project ( int N, float * u, float * v, float * w, float * p, float * div )
{
	int i, j, l, k;
	float h;
	h = 1.0/N;
	for ( i=1 ; i<=N ; i++ ) {
		for ( j=1 ; j<=N ; j++ ) {
			for ( l=1 ; l<=N ; l++ )
			{
				div[IX(i,j,l)] = -0.5*h*(
						u[IX(i+1,j,l)]-u[IX(i-1,j,l)]+
						v[IX(i,j+1,l)]-v[IX(i,j-1,l)]+
						w[IX(i,j,l+1)]-w[IX(i,j,l-1)]);
				p[IX(i,j,l)] = 0;
			}
		}
	}
	set_bnd ( N, 0, div ); set_bnd ( N, 0, p );
	for ( k=0 ; k<20 ; k++ ) {
		for ( i=1 ; i<=N ; i++ ) {
			for ( j=1 ; j<=N ; j++ ) {
				for ( l=1 ; l<=N ; l++ ) {
					p[IX(i,j,l)] = (div[IX(i,j,l)]
						+p[IX(i-1,j,l)]+p[IX(i+1,j,l)]
						+p[IX(i,j-1,l)]+p[IX(i,j+1,l)]
						+p[IX(i,j,l-1)]+p[IX(i,j,l+1)])/6;
				}
			}
		}
		set_bnd ( N, 0, p );
	}
	for ( i=1 ; i<=N ; i++ ) {
		for ( j=1 ; j<=N ; j++ ) {
			for ( l=1 ; l<=N ; l++ ) {
				u[IX(i,j,l)] -= 0.5*(p[IX(i+1,j,l)]-p[IX(i-1,j,l)])/h;
				v[IX(i,j,l)] -= 0.5*(p[IX(i,j+1,l)]-p[IX(i,j-1,l)])/h;
				w[IX(i,j,l)] -= 0.5*(p[IX(i,j,l+1)]-p[IX(i,j,l-1)])/h;
			}
		}
	}
	set_bnd ( N, 1, u ); set_bnd ( N, 2, v ); set_bnd ( N, 3, w );
}


void diffuse ( int N, int b, float * x, float * x0, float diff, float dt )
{
	int i, j, l, k;
	float a=dt*diff*N*N;
	for ( k=0 ; k<20 ; k++ ) 
	{
		for ( i=1 ; i<=N ; i++ )
		{
			for ( j=1 ; j<=N ; j++ ) 
			{
				for ( l=1 ; l<=N ; l++ )
				{
					float t1 = x[IX(i-1,j,l)]+x[IX(i+1,j,l)];
					float t2 = x[IX(i,j-1,l)]+x[IX(i,j+1,l)];
					float t3 = x[IX(i,j,l-1)]+x[IX(i,j,l+1)];

					x[IX(i,j,l)] = (x0[IX(i,j,l)] +	a*(t1 + t2 + t3)) / (1+6*a);;
			
				}
			}
		}
		set_bnd ( N, b, x );
	}
}

void advect ( int N, int b, float * d, float * d0, float * u, float * v, float * w, float dt )
{
	int i, j, l, i0, j0, l0, i1, j1, l1;
	float x, y, z, s0, t0, r0, s1, t1, r1, dt0;
	dt0 = dt*N;
	for ( i=1 ; i<=N ; i++ ) 
	{
		for ( j=1 ; j<=N ; j++ )
		{
			for ( l=1 ; l<=N ; l++ )
			{
				//Trace velocity back to corrdinates (x, y, z)
				//(current cell) - (velocity vector) = (source point)
				x = (float)i-dt0*u[IX(i,j,l)];
				y = (float)j-dt0*v[IX(i,j,l)];
				z = (float)l-dt0*w[IX(i,j,l)];

				//1. Prevent going out of bounds
				//2. Convert coordinates (xyz : float) to cells (ijl : int)
				//(i1,j1,l1) are (i0,j0,l1) + 1
				if (x<0.5) x=0.5; if (x>N+0.5) x=N+ 0.5; i0=(int)x; i1=i0+1;
				if (y<0.5) y=0.5; if (y>N+0.5) y=N+ 0.5; j0=(int)y; j1=j0+1;
				if (z<0.5) z=0.5; if (z>N+0.5) z=N+ 0.5; l0=(int)z; l1=l0+1;

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
		}
	}
	set_bnd ( N, b, d );
}


void dens_step( int N, float * x, float * x0, float * u, float * v, float * w, float diff, float dt )
{         
	add_source ( N, x, x0, dt );
	SWAP ( x0,x ); diffuse ( N, 0, x, x0, diff, dt );
	SWAP ( x0,x ); advect ( N, 0, x, x0, u, v, w, dt );
}

void vel_step ( int N, float * u, float * v, float * w, float* u0, float * v0, float * w0, float visc, float dt )                             
{                                                  
	add_source ( N, u, u0, dt ); add_source ( N, v, v0, dt ); add_source ( N, w, w0, dt );
	SWAP ( u0, u ); diffuse ( N, 1, u, u0, visc, dt);
	SWAP ( v0, v ); diffuse ( N, 2, v, v0, visc, dt);
	SWAP ( w0, w ); diffuse ( N, 3, w, w0, visc, dt);
	project ( N, u, v, w, u0, v0 );                    
	SWAP ( u0, u ); SWAP ( v0, v ); SWAP ( w0, w );
	advect ( N, 1, u, u0, u0, v0, w0, dt );
	advect ( N, 2, v, v0, u0, v0, w0, dt );
	advect ( N, 3, w, w0, u0, v0, w0, dt );
	project ( N, u, v, w, u0, v0 );
}




