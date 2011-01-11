all: osceleton
osx: osceleton-osx

liboscpack:
	cd oscpack; make

osceleton: liboscpack
	gcc OSCeleton/OSCeleton.cpp -Ioscpack -I/usr/include/ni -lOpenNI oscpack/ip/*.o oscpack/ip/posix/*.o oscpack/osc/*.o -o osceleton

osceleton-osx: liboscpack
	g++ OSCeleton/OSCeleton.cpp -Ioscpack -I/usr/include/ni -lOpenNI -lstdc++ oscpack/ip/*.o oscpack/ip/posix/*.o oscpack/osc/*.o -o osceleton-osx

clean:
	rm -f osceleton osceleton.app ; cd oscpack ; make clean
