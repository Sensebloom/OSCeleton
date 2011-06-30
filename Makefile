all: osceleton

liblo:
	cd liblo-0.26;./configure;make

osceleton: liblo
	g++ src/OSCeleton.cpp src/viewer.cpp -O3 -Wno-write-strings -Iliblo-0.26 -I/usr/X11/include -I/usr/include/ni -lOpenNI -lstdc++ -L/usr/X11/lib -lGL -lGLU -lglut liblo-0.26/src/.libs/*.o -o osceleton

clean:
	rm -f osceleton ; cd liblo-0.26; make clean
