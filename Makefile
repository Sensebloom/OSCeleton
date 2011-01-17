all: osceleton

liboscpack:
	cd oscpack; make

osceleton: liboscpack
	g++ src/OSCeleton.cpp -Wno-write-strings -Ioscpack -I/usr/include/ni -lOpenNI -lstdc++ oscpack/ip/*.o oscpack/ip/posix/*.o oscpack/osc/*.o -o osceleton

clean:
	rm -f osceleton ; cd oscpack ; make clean
