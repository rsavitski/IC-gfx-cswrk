#include <GL/glut.h>


#include <stdlib.h>

#include <fstream>
#include <iostream>


using namespace std;


void init() 
{	
	cout << "init" << endl;
	
	glClearColor (0.0, 0.0, 0.0, 0.0);
	
	glShadeModel (GL_SMOOTH);	// interpolation
	
	glEnable(GL_DEPTH_TEST); // Z-buffer
	
	/*
	// Enable lighting
	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
	glLightfv(GL_LIGHT0, GL_AMBIENT,  LightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  LightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, LightSpecular);

	// Set material parameters
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  MaterialSpecular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, MaterialShininess);
	 */
}

void display(void)
{
	cout << "display" << endl;
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	
	glBegin(GL_POLYGON);
		glColor3f(1,0,0);
		glVertex3f(1,1,1);
		glColor3f(0,1,0);
		glVertex3f(0,1,1);
		glColor3f(0,0,1);
		glVertex3f(1,0,1);
	glEnd();

	glutSwapBuffers();
	/*
	   for (all polygons)
	   glBegin(GL_POLYGON);
	   for (all vertices of polygon)
	// Define texture coordinates of vertex
	glTexCoord2f(...);
	// Define normal of vertex
	glNormal3f(...);
	// Define coordinates of vertex
	glVertex3f(...);
	}
	glEnd();
	}
	glFlush ();
	*/
	 
	}

void reshape (int w, int h)
{
	cout << "reshape" << endl;

	glViewport (0, 0, (GLsizei) w, (GLsizei) h); 

	   glMatrixMode (GL_PROJECTION);
	   glLoadIdentity();
	   gluPerspective(90, 1, 0.2, 10);
	   glMatrixMode (GL_MODELVIEW);
	   glLoadIdentity();
	   gluLookAt(0, 0, 3, 
	   			 0, 0, 0,
	   			 0, 1, 0);
	
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key) 
	{
		case 27: // ESC
			exit(0);
			break;
	}
}

int main(int argc, char** argv)
{
	// Initialize graphics window
	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);	// double buffered

	glutInitWindowSize (768, 768); 
	glutInitWindowPosition (-1, -1);	// window manager determines position
	glutCreateWindow ("GFX");

	// Initialise state
	init();

	// Attach callbacks
	glutDisplayFunc(display); 
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);

	// Start rendering 
	glutMainLoop();
}
