all: osceleton

liboscpack:
	cd oscpack; make

osceleton: liboscpack
	g++ src/OSCeleton.cpp src/viewer.cpp -O3 -Wno-write-strings -Ioscpack -I/usr/include/ni -lOpenNI -lstdc++ -lGL -lGLU -lglut oscpack/ip/*.o oscpack/ip/posix/*.o oscpack/osc/*.o -o osceleton

clean:
	rm -f osceleton ; cd oscpack ; make clean
