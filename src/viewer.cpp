
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include "viewer.h"



int window; // GLUT window id
int width;
int height;
GLuint tex_id;
int buf_size;
GLubyte *buf;


/* The main drawing function. */
void draw() {
	const XnDepthPixel *data = depthMD.Data();
	GLubyte tmp;
	for (int i = 0; i < width*height; i++) {
		tmp = (10000 - data[i]) / 80;
		//printf("tmp = %d\n", tmp);
		buf[i*3] = tmp;
		buf[i*3+1] = tmp;
		buf[i*3+2] = tmp;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);


	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// glLoadIdentity();

	// glTranslatef(0.0f,0.0f,-3.0f);

	// 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image,
    // border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.

	glBindTexture(GL_TEXTURE_2D, tex_id);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	// draw a square (quadrilateral)
	glBegin(GL_QUADS);				// start drawing a polygon (4 sided)
	glTexCoord2f(0.0f, 0.0f); glVertex2i(0, 0);		// Top Left
	glTexCoord2f(1.0f, 0.0f); glVertex2i(width, 0);		// Top Right
	glTexCoord2f(1.0f, 1.0f); glVertex2i(width, height);		// Bottom Right
	glTexCoord2f(0.0f, 1.0f); glVertex2i(0, height);		// Bottom Left
	glEnd();					// done with the polygon

	// swap buffers to display, since we're double buffered.
	glutSwapBuffers();
}



void init_tex() {
	// Create Texture
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
}



void init_window(int argc, char **argv, int w, int h, void main_loop(void)) {
	width = w;
	height = h;
	buf_size = 3*width*height*sizeof(GLubyte);
	buf = (GLubyte*)malloc(buf_size);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(0, 0);
	window = glutCreateWindow("OSCeleton");

	/* Register the function to do all our OpenGL drawing. */
	glutDisplayFunc(main_loop);

	/* Go fullscreen.  This is the soonest we could possibly go fullscreen. */
	//glutFullScreen();

	/* Even if there are no events, redraw our gl scene. */
	glutIdleFunc(main_loop);

	/* Register the function called when our window is resized. */
	//glutReshapeFunc(&ReSizeGLScene);

	/* Register the function called when the keyboard is pressed. */
	//glutKeyboardFunc(&keyPressed);



	gluOrtho2D(0.0, width, height, 0.0);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	init_tex();

	//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	//glClearDepth(1.0);
	//glDepthFunc(GL_LESS);
	//glEnable(GL_DEPTH_TEST);
	//glShadeModel(GL_SMOOTH);

	//glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();

	//gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);	// Calculate The Aspect Ratio Of The Window

	//glMatrixMode(GL_MODELVIEW);
}
