Parallel, auto-tuning fluid simulations
==============
by Olafs Vandans

**OpenCL can't be statically linked, you have to [get it](https://wiki.tiker.net/OpenCLHowTo) and install the relevant ICD for your system.**

Included files and directories
----------

* fluid2d  
An early implementation of the 2D fluid solver, following Jos Stam's paper on stable fluids. Calling "make" should compile it if gcc is present. Density is added with the left mouse button, velocity - with the right.

* GPUTracer-0.01.zip  
GPUTracer served as the basis for the raycaster component of this project. It is written in C# and OpenCL, but the relevant CL file is the archive's src/GPUCaster/GPUCaster/RayCaster.cl

* fluid3d  
The finished implementation, as described in the project. Refer to the implementation section of the report for compiling notes, command line arguments and keyboard controls. Generally, just calling "make" should compile it fine under GCC. For OpenCL 1.1, call "make opencl11". There are dependencies on GLFW (http://www.glfw.org/) and any OpenCL SDK (though only AMD and nVidia were tested). The architecture of the program is relatively simple and explained well by the names of the components. Code comments should provide extra guidance on what the program does.

* fluid3d/scripts  
Python and gnuplot scripts used to generate the graphs for the test runs. Log files from some of the test runs are in the fluid3d/scripts/archive directory. To generate a graph, you would run the program with logging turned on (default). This would output two files in the current directory - frames.log and profile.log, containing a history of frame times and kernel calls. These files can be piped into the relevant python script, which generates a corresponding .dat file for gnuplot, and then calls gnuplot with the correct parameters.

Scripts (for generating pretty pictures)
------------
* kernels.py - draw a histogram of kernel execution times  
```
$ kernels.py < ../profile.log			- outputs just the .dat file
$ kernels.py < ../profile.log -draw		- draws on the x11 or png terminal (specify in kernels.gp)
$ kernels.py < ../profile.log -draw -normalize	- draws a normalized version
```

* normalize.py - draw the same as above, but normalized; this is usually called by the above script, not directly

* param.py - draws and fits a graph of frame times (y-value), taking 20 frame intervals per parameter (x-value). The gp files are param_resolution.gp and param_precision.gp with different model functions.  
```
$ param.py < ../frames.log
```			

* fractions.py - outputs the aggregate times the simulation has spent on the CPU and GPU for both memory operations and actual processing. This is used in calculating the sequential fraction of the computation. **You'd need to run the program with `COMPUTE_PROFILE=1` and `COMPUTE_PROFILE_CSV=1` in the environment.**  
```
$ fractions.py < ../opencl_profile_0.log
```

Documentation
----------------
Not every referenced paper is included, only the most important ones. Among them are:  

* The project's final and interim reports
* Jos Stam's stable fluid solver
* Eric Blanchard's attempt to convert it to OpenCL
* Presentations from project meetings
* AMD APP SDK and OpenCL programming guides, including the C++ wrapper
* gnuplot fitting tutorial, found at http://www.sfu.ca/phys/231s/resources/gnuplot/gnuplot_fit_tutorial.pdf
