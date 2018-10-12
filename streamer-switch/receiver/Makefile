CFLAGS = -std=c++11
main:	main.cpp
	g++ -o main `pkg-config --cflags opencv` $(CFLAGS) main.cpp `pkg-config --libs opencv`
clean:
	rm main
