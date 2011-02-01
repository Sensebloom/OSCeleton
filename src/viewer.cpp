#include "common.h"

#include <GL/gl.h>
#include <GL/glu.h>



int window; // GLUT window id
int width;
int height;
GLuint tex_id;
int buf_size;
GLubyte *buf;



void draw() {
	const XnDepthPixel *data = depthMD.Data();
	GLubyte tmp;

	for (int i = 0; i < width*height; i++) {
		tmp = (10000 - data[i]) >> 3;
		buf[i*3] = tmp;
		buf[i*3+1] = tmp;
		buf[i*3+2] = tmp;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);

	glBindTexture(GL_TEXTURE_2D, tex_id);
	glColor4f(1.0, 1.0, 1.0, 1.0);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2i(0, 0);
	glTexCoord2f(1.0f, 0.0f); glVertex2i(width, 0);
	glTexCoord2f(1.0f, 1.0f); glVertex2i(width, height);
	glTexCoord2f(0.0f, 1.0f); glVertex2i(0, height);
	glEnd();

	glutSwapBuffers();
}



void init_tex() {
	// Create Texture
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
}



void reshape_window(int w, int h) {
	glutReshapeWindow(width, height);
}



void key_pressed(unsigned char key, int x, int y) {
	if (key == 27)
		glutDestroyWindow(window);
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

	glutDisplayFunc(main_loop);
	glutIdleFunc(main_loop);
	glutReshapeFunc(reshape_window);
	glutKeyboardFunc(key_pressed);

	gluOrtho2D(0.0, width, height, 0.0);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	init_tex();
}
