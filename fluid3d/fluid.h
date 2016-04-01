//Plain C version of the fluid solver (for reference)

#include "main.h"

void add_source ( int N, float * x, float * s, float dt );
void set_bnd ( int N, int b, float * x ) ;
void project ( int N, float * u, float * v, float* w, float * p, float * div );
void diffuse ( int N, int b, float * x, float * x0, float diff, float dt );
void advect ( int N, int b, float * d, float * d0, float * u, float * v, float* w, float dt );
void dens_step( int N, float * x, float * x0, float * u, float * v, float* w, float diff, float dt );
void vel_step ( int N, float * u, float * v, float* w, float* u0, float * v0, float* w0, float visc, float dt );
void draw_dens(int N, float* dens);
