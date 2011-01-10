all: osceleton
osx: osceleton-osx

liboscpack:
	cd oscpack; make

osceleton: liboscpack
	gcc OSCeleton/OSCeleton.cpp -Ioscpack -I/usr/include/ni -lOpenNI oscpack/ip/*.o oscpack/ip/posix/*.o oscpack/osc/*.o -o osceleton

liboscpack-osx:
	cd oscpack; make -f Makefile.osx

osceleton-osx: liboscpack-osx
	g++-4.0 OSCeleton/OSCeleton.cpp -Ioscpack -I/usr/include/ni -lOpenNI -lstdc++ oscpack/ip/*.o oscpack/ip/posix/*.o oscpack/osc/*.o -o osceleton.app

clean:
	rm -f osceleton osceleton.app ; cd oscpack ; make clean
