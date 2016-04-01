default: main.cpp simulation.cpp tuner.cpp graphics.cpp raycaster.cpp File.cpp
	g++ -O0 -pipe main.cpp graphics.cpp simulation.cpp tuner.cpp raycaster.cpp File.cpp -std=c++11 -w -lGL -lGLU -lOpenCL `pkg-config --static --libs glfw3`

opencl11: main.cpp simulation.cpp tuner.cpp graphics.cpp raycaster.cpp File.cpp
	g++ -Dopencl11 -O0 -pipe main.cpp graphics.cpp simulation.cpp tuner.cpp raycaster.cpp File.cpp -std=c++11 -w -lGL -lGLU -lOpenCL `pkg-config --static --libs glfw3`

r: 
	./a.out

d:
	gdb -tui a.out
