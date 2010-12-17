all: osceleton

liboscpack.so:
	cd oscpack; make lib

osceleton: liboscpack.so
	gcc OSCeleton/OSCeleton.cpp -Ioscpack -I/usr/include/ni -lOpenNI oscpack/ip/*.o oscpack/ip/posix/*.o oscpack/osc/*.o -o osceleton

clean:
	rm -f osceleton ; cd oscpack ; make clean
