VERSION=1.0
#for g++
CFLAGS=-g -Wall  -c  -std=c++11 -O0
CXX = g++


INSTALL_PATH =  /usr/local/lib

main : main.cpp PanoramaRemapperGen.o BirdsEyeViewRemapperGen.o RingStitch.o SideBySideStitch.o matrix.o
	$(CXX) -g -Wall -std=c++11 -o main `pkg-config --cflags opencv` main.cpp `pkg-config --libs opencv`  PanoramaRemapperGen.o BirdsEyeViewRemapperGen.o RingStitch.o SideBySideStitch.o matrix.o -lglfw3 -lGLEW -lGL -lXrandr -lXinerama -lXcursor -lXi -lXxf86vm -lX11 -lpthread -lrt -lm -ldl 
	

matrix.o:	matrix.cpp OmnidirectionalCamera.hpp
	$(CXX) $(CFLAGS) `pkg-config --cflags opencv` matrix.cpp `pkg-config --libs opencv` -o matrix.o

PanoramaRemapperGen.o:	PanoramaRemapperGen.cpp OmnidirectionalCamera.hpp
	$(CXX) $(CFLAGS) `pkg-config --cflags opencv` PanoramaRemapperGen.cpp `pkg-config --libs opencv` -o PanoramaRemapperGen.o

BirdsEyeViewRemapperGen.o: BirdsEyeViewRemapperGen.cpp OmnidirectionalCamera.hpp
	$(CXX) $(CFLAGS) `pkg-config --cflags opencv` BirdsEyeViewRemapperGen.cpp `pkg-config --libs opencv` -o BirdsEyeViewRemapperGen.o

RingStitch.o:  RingStitch.cpp OmnidirectionalCamera.hpp
	$(CXX) $(CFLAGS) `pkg-config --cflags opencv` RingStitch.cpp `pkg-config --libs opencv` -o RingStitch.o

SideBySideStitch.o:   SideBySideStitch.cpp OmnidirectionalCamera.hpp
	$(CXX) $(CFLAGS) `pkg-config --cflags opencv` SideBySideStitch.cpp `pkg-config --libs opencv` -o SideBySideStitch.o	

clean:
	rm main
	rm *.o
