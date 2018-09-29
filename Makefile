main:
	g++  `pkg-config --cflags opencv`  main.cpp -std=c++11 -g `pkg-config --libs opencv` -lglfw3 -lGLEW -lGL -lXrandr -lXinerama -lXcursor -lXi -lXxf86vm -lX11 -lpthread -lrt -lm -ldl 
