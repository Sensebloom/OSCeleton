all: osceleton osc2file osc2text file2osc

liblo:
	cd liblo-0.26-modified;./configure;make

osceleton: liblo
	g++ src/OSCeleton.cpp src/viewer.cpp -O3 -Wno-write-strings -Iliblo-0.26-modified -I/usr/X11/include -I/usr/include/ni -lOpenNI -lstdc++ -L/usr/X11/lib -lGL -lGLU -lglut -lpthread liblo-0.26-modified/src/.libs/*.o -o osceleton

osc2file: liblo
	g++ osc_tools/osc2file.c -O3 -Wno-write-strings -Iliblo-0.26-modified -lpthread liblo-0.26-modified/src/.libs/*.o -o osc2file

osc2text: liblo
	g++ osc_tools/osc2text.c -O3 -Wno-write-strings -Iliblo-0.26-modified -lpthread liblo-0.26-modified/src/.libs/*.o -o osc2text

file2osc: liblo
	g++ osc_tools/file2osc.c -O3 -Wno-write-strings -Iliblo-0.26-modified -lpthread liblo-0.26-modified/src/.libs/*.o -o file2osc

clean:
	rm -f osceleton; rm -f os2file; rm -f file2osc ; cd liblo-0.26-modified; make clean
